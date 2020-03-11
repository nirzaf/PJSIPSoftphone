/* $Id: ssl_sock_ossl.c 5885 2018-09-18 12:07:50Z riza $ */
/* 
 * Copyright (C) 2009-2011 Teluu Inc. (http://www.teluu.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#include <pj/ssl_sock.h>
#include <pj/activesock.h>
#include <pj/compat/socket.h>
#include <pj/assert.h>
#include <pj/errno.h>
#include <pj/file_access.h>
#include <pj/list.h>
#include <pj/lock.h>
#include <pj/log.h>
#include <pj/math.h>
#include <pj/os.h>
#include <pj/pool.h>
#include <pj/string.h>
#include <pj/timer.h>


/* Only build when PJ_HAS_SSL_SOCK is enabled and when the backend is
 * OpenSSL.
 */
#if defined(PJ_HAS_SSL_SOCK) && PJ_HAS_SSL_SOCK != 0 && \
    (PJ_SSL_SOCK_IMP == PJ_SSL_SOCK_IMP_OPENSSL)

#define THIS_FILE		"ssl_sock_ossl.c"

/* Workaround for ticket #985 and #1930 */
#ifndef PJ_SSL_SOCK_DELAYED_CLOSE_TIMEOUT
#   define PJ_SSL_SOCK_DELAYED_CLOSE_TIMEOUT	500
#endif

/* 
 * Include OpenSSL headers 
 */
#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#if !defined(OPENSSL_NO_DH)
#   include <openssl/dh.h>
#endif

#include <openssl/rand.h>
#include <openssl/opensslconf.h>
#include <openssl/opensslv.h>

#if defined(LIBRESSL_VERSION_NUMBER)
#	define USING_LIBRESSL 1
#else
#	define USING_LIBRESSL 0
#endif

#if !USING_LIBRESSL && !defined(OPENSSL_NO_EC) \
	&& OPENSSL_VERSION_NUMBER >= 0x1000200fL

#   include <openssl/obj_mac.h>

static const unsigned nid_cid_map[] = {
    NID_sect163k1,              /* sect163k1 (1) */
    NID_sect163r1,              /* sect163r1 (2) */
    NID_sect163r2,              /* sect163r2 (3) */
    NID_sect193r1,              /* sect193r1 (4) */
    NID_sect193r2,              /* sect193r2 (5) */
    NID_sect233k1,              /* sect233k1 (6) */
    NID_sect233r1,              /* sect233r1 (7) */
    NID_sect239k1,              /* sect239k1 (8) */
    NID_sect283k1,              /* sect283k1 (9) */
    NID_sect283r1,              /* sect283r1 (10) */
    NID_sect409k1,              /* sect409k1 (11) */
    NID_sect409r1,              /* sect409r1 (12) */
    NID_sect571k1,              /* sect571k1 (13) */
    NID_sect571r1,              /* sect571r1 (14) */
    NID_secp160k1,              /* secp160k1 (15) */
    NID_secp160r1,              /* secp160r1 (16) */
    NID_secp160r2,              /* secp160r2 (17) */
    NID_secp192k1,              /* secp192k1 (18) */
    NID_X9_62_prime192v1,       /* secp192r1 (19) */
    NID_secp224k1,              /* secp224k1 (20) */
    NID_secp224r1,              /* secp224r1 (21) */
    NID_secp256k1,              /* secp256k1 (22) */
    NID_X9_62_prime256v1,       /* secp256r1 (23) */
    NID_secp384r1,              /* secp384r1 (24) */
    NID_secp521r1,              /* secp521r1 (25) */
    NID_brainpoolP256r1,        /* brainpoolP256r1 (26) */
    NID_brainpoolP384r1,        /* brainpoolP384r1 (27) */
    NID_brainpoolP512r1         /* brainpoolP512r1 (28) */
};

static unsigned get_cid_from_nid(unsigned nid)
{
    unsigned i, cid = 0;
    for (i=0; i<PJ_ARRAY_SIZE(nid_cid_map); ++i) {
	if (nid == nid_cid_map[i]) {
	    cid = i+1;
	    break;
	}
    }
    return cid;
}

static unsigned get_nid_from_cid(unsigned cid)
{
    if ((cid == 0) || (cid > PJ_ARRAY_SIZE(nid_cid_map)))
	return 0;

    return nid_cid_map[cid-1];
}

#endif


#if !USING_LIBRESSL && OPENSSL_VERSION_NUMBER >= 0x10100000L
#  define OPENSSL_NO_SSL2	    /* seems to be removed in 1.1.0 */
#  define M_ASN1_STRING_data(x)	    ASN1_STRING_get0_data(x)
#  define M_ASN1_STRING_length(x)   ASN1_STRING_length(x)
#  if defined(OPENSSL_API_COMPAT) && OPENSSL_API_COMPAT >= 0x10100000L
#     define X509_get_notBefore(x)  X509_get0_notBefore(x)
#     define X509_get_notAfter(x)   X509_get0_notAfter(x)
#  endif
#else
#  define SSL_CIPHER_get_id(c)	    (c)->id
#  define SSL_set_session(ssl, s)   (ssl)->session = (s)
#endif


#ifdef _MSC_VER
#  if OPENSSL_VERSION_NUMBER >= 0x10100000L
#    pragma comment(lib, "libcrypto")
#    pragma comment(lib, "libssl")
#    pragma comment(lib, "crypt32")
#  else
#    pragma comment(lib, "libeay32")
#    pragma comment(lib, "ssleay32")
#  endif
#  define strerror_r(errno,buf,len) strerror_s(buf,len,errno)
#endif


/* Suppress compile warning of OpenSSL deprecation (OpenSSL is deprecated
 * since MacOSX 10.7).
 */
#if defined(PJ_DARWINOS) && PJ_DARWINOS==1
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif


/*
 * SSL/TLS state enumeration.
 */
enum ssl_state {
    SSL_STATE_NULL,
    SSL_STATE_HANDSHAKING,
    SSL_STATE_ESTABLISHED,
    SSL_STATE_ERROR
};

/*
 * Internal timer types.
 */
enum timer_id
{
    TIMER_NONE,
    TIMER_HANDSHAKE_TIMEOUT,
    TIMER_CLOSE
};

/*
 * Structure of SSL socket read buffer.
 */
typedef struct read_data_t
{
    void		 *data;
    pj_size_t		  len;
} read_data_t;

/*
 * Get the offset of pointer to read-buffer of SSL socket from read-buffer
 * of active socket. Note that both SSL socket and active socket employ 
 * different but correlated read-buffers (as much as async_cnt for each),
 * and to make it easier/faster to find corresponding SSL socket's read-buffer
 * from known active socket's read-buffer, the pointer of corresponding 
 * SSL socket's read-buffer is stored right after the end of active socket's
 * read-buffer.
 */
#define OFFSET_OF_READ_DATA_PTR(ssock, asock_rbuf) \
					(read_data_t**) \
					((pj_int8_t*)(asock_rbuf) + \
					ssock->param.read_buffer_size)

/*
 * Structure of SSL socket write data.
 */
typedef struct write_data_t {
    PJ_DECL_LIST_MEMBER(struct write_data_t);
    pj_ioqueue_op_key_t	 key;
    pj_size_t 	 	 record_len;
    pj_ioqueue_op_key_t	*app_key;
    pj_size_t 	 	 plain_data_len;
    pj_size_t 	 	 data_len;
    unsigned		 flags;
    union {
	char		 content[1];
	const char	*ptr;
    } data;
} write_data_t;

/*
 * Structure of SSL socket write buffer (circular buffer).
 */
typedef struct send_buf_t {
    char		*buf;
    pj_size_t		 max_len;    
    char		*start;
    pj_size_t		 len;
} send_buf_t;

/*
 * Secure socket structure definition.
 */
struct pj_ssl_sock_t
{
    pj_pool_t		 *pool;
    pj_ssl_sock_t	 *parent;
    pj_ssl_sock_param	  param;
    pj_ssl_sock_param	  newsock_param;
    pj_ssl_cert_t	 *cert;
    
    pj_ssl_cert_info	  local_cert_info;
    pj_ssl_cert_info	  remote_cert_info;

    pj_bool_t		  is_server;
    enum ssl_state	  ssl_state;
    pj_ioqueue_op_key_t	  handshake_op_key;
    pj_timer_entry	  timer;
    pj_status_t		  verify_status;

    unsigned long	  last_err;

    pj_sock_t		  sock;
    pj_activesock_t	 *asock;

    pj_sockaddr		  local_addr;
    pj_sockaddr		  rem_addr;
    int			  addr_len;
    
    pj_bool_t		  read_started;
    pj_size_t		  read_size;
    pj_uint32_t		  read_flags;
    void		**asock_rbuf;
    read_data_t		 *ssock_rbuf;

    write_data_t	  write_pending;/* list of pending write to OpenSSL */
    write_data_t	  write_pending_empty; /* cache for write_pending   */
    pj_bool_t		  flushing_write_pend; /* flag of flushing is ongoing*/
    send_buf_t		  send_buf;
    write_data_t 	  send_buf_pending; /* send buffer is full but some
					     * data is queuing in wbio.     */
    write_data_t	  send_pending;	/* list of pending write to network */
    pj_lock_t		 *write_mutex;	/* protect write BIO and send_buf   */

    SSL_CTX		 *ossl_ctx;
    SSL			 *ossl_ssl;
    BIO			 *ossl_rbio;
    BIO			 *ossl_wbio;
};


/*
 * Certificate/credential structure definition.
 */
struct pj_ssl_cert_t
{
    pj_str_t CA_file;
    pj_str_t CA_path;
    pj_str_t cert_file;
    pj_str_t privkey_file;
    pj_str_t privkey_pass;

    /* Certificate buffer. */
    pj_ssl_cert_buffer CA_buf;
    pj_ssl_cert_buffer cert_buf;
    pj_ssl_cert_buffer privkey_buf;
};


static write_data_t* alloc_send_data(pj_ssl_sock_t *ssock, pj_size_t len);
static void free_send_data(pj_ssl_sock_t *ssock, write_data_t *wdata);
static pj_status_t flush_delayed_send(pj_ssl_sock_t *ssock);

/*
 *******************************************************************
 * Static/internal functions.
 *******************************************************************
 */

/**
 * Mapping from OpenSSL error codes to pjlib error space.
 */

#define PJ_SSL_ERRNO_START		(PJ_ERRNO_START_USER + \
					 PJ_ERRNO_SPACE_SIZE*6)

#define PJ_SSL_ERRNO_SPACE_SIZE		PJ_ERRNO_SPACE_SIZE

/* Expected maximum value of reason component in OpenSSL error code */
#define MAX_OSSL_ERR_REASON		1200


static char *SSLErrorString (int err)
{
    switch (err) {
    case SSL_ERROR_NONE:
	return "SSL_ERROR_NONE";
    case SSL_ERROR_ZERO_RETURN:
	return "SSL_ERROR_ZERO_RETURN";
    case SSL_ERROR_WANT_READ:
	return "SSL_ERROR_WANT_READ";
    case SSL_ERROR_WANT_WRITE:
	return "SSL_ERROR_WANT_WRITE";
    case SSL_ERROR_WANT_CONNECT:
	return "SSL_ERROR_WANT_CONNECT";
    case SSL_ERROR_WANT_ACCEPT:
	return "SSL_ERROR_WANT_ACCEPT";
    case SSL_ERROR_WANT_X509_LOOKUP:
	return "SSL_ERROR_WANT_X509_LOOKUP";
    case SSL_ERROR_SYSCALL:
	return "SSL_ERROR_SYSCALL";
    case SSL_ERROR_SSL:
	return "SSL_ERROR_SSL";
    default:
	return "SSL_ERROR_UNKNOWN";
    }
}

#define ERROR_LOG(msg, err) \
    PJ_LOG(2,("SSL", "%s (%s): Level: %d err: <%lu> <%s-%s-%s> len: %d", \
	      msg, action, level, err, \
	      (ERR_lib_error_string(err)? ERR_lib_error_string(err): "???"), \
	      (ERR_func_error_string(err)? ERR_func_error_string(err):"???"),\
	      (ERR_reason_error_string(err)? \
	       ERR_reason_error_string(err): "???"), len));

static void SSLLogErrors(char * action, int ret, int ssl_err, int len)
{
    char *ssl_err_str = SSLErrorString(ssl_err);

    if (!action) {
	action = "UNKNOWN";
    }

    switch (ssl_err) {
    case SSL_ERROR_SYSCALL:
    {
	unsigned long err2 = ERR_get_error();
	if (err2) {
	    int level = 0;
	    while (err2) {
	        ERROR_LOG("SSL_ERROR_SYSCALL", err2);
		level++;
		err2 = ERR_get_error();
	    }
	} else if (ret == 0) {
	    /* An EOF was observed that violates the protocol */

	    /* The TLS/SSL handshake was not successful but was shut down
	     * controlled and by the specifications of the TLS/SSL protocol.
	     */
	} else if (ret == -1) {
	    /* BIO error - look for more info in errno... */
	    char errStr[250] = "";
	    strerror_r(errno, errStr, sizeof(errStr));
	    /* for now - continue logging these if they occur.... */
	    PJ_LOG(4,("SSL", "BIO error, SSL_ERROR_SYSCALL (%s): "
	    		     "errno: <%d> <%s> len: %d",
		      	     action, errno, errStr, len));
	} else {
	    /* ret!=0 & ret!=-1 & nothing on error stack - is this valid??? */
	    PJ_LOG(2,("SSL", "SSL_ERROR_SYSCALL (%s) ret: %d len: %d",
		      action, ret, len));
	}
	break;
    }
    case SSL_ERROR_SSL:
    {
	unsigned long err2 = ERR_get_error();
	int level = 0;

	while (err2) {
	    ERROR_LOG("SSL_ERROR_SSL", err2);
	    level++;
	    err2 = ERR_get_error();
	}
	break;
    }
    default:
	PJ_LOG(2,("SSL", "%lu [%s] (%s) ret: %d len: %d",
		  ssl_err, ssl_err_str, action, ret, len));
	break;
    }
}


static pj_status_t GET_STATUS_FROM_SSL_ERR(unsigned long err)
{
    pj_status_t status;

    /* OpenSSL error range is much wider than PJLIB errno space, so
     * if it exceeds the space, only the error reason will be kept.
     * Note that the last native error will be kept as is and can be
     * retrieved via SSL socket info.
     */
    status = ERR_GET_LIB(err)*MAX_OSSL_ERR_REASON + ERR_GET_REASON(err);
    if (status > PJ_SSL_ERRNO_SPACE_SIZE)
	status = ERR_GET_REASON(err);

    status += PJ_SSL_ERRNO_START;
    return status;
}

/* err contains ERR_get_error() status */
static pj_status_t STATUS_FROM_SSL_ERR(char *action, pj_ssl_sock_t *ssock,
				       unsigned long err)
{
    int level = 0;
    int len = 0; //dummy

    ERROR_LOG("STATUS_FROM_SSL_ERR", err);
    level++;

    /* General SSL error, dig more from OpenSSL error queue */
    if (err == SSL_ERROR_SSL) {
	err = ERR_get_error();
	ERROR_LOG("STATUS_FROM_SSL_ERR", err);
    }

    ssock->last_err = err;
    return GET_STATUS_FROM_SSL_ERR(err);
}

/* err contains SSL_get_error() status */
static pj_status_t STATUS_FROM_SSL_ERR2(char *action, pj_ssl_sock_t *ssock,
					int ret, int err, int len)
{
    unsigned long ssl_err = err;

    if (err == SSL_ERROR_SSL) {
	ssl_err = ERR_peek_error();
    }

    /* Dig for more from OpenSSL error queue */
    SSLLogErrors(action, ret, err, len);

    ssock->last_err = ssl_err;
    return GET_STATUS_FROM_SSL_ERR(ssl_err);
}

static pj_status_t GET_SSL_STATUS(pj_ssl_sock_t *ssock)
{
    return STATUS_FROM_SSL_ERR("status", ssock, ERR_get_error());
}


/*
 * Get error string of OpenSSL.
 */
static pj_str_t ssl_strerror(pj_status_t status, 
			     char *buf, pj_size_t bufsize)
{
    pj_str_t errstr;
    unsigned long ssl_err = status;

    if (ssl_err) {
	unsigned long l, r;
	ssl_err -= PJ_SSL_ERRNO_START;
	l = ssl_err / MAX_OSSL_ERR_REASON;
	r = ssl_err % MAX_OSSL_ERR_REASON;
	ssl_err = ERR_PACK(l, 0, r);
    }

#if defined(PJ_HAS_ERROR_STRING) && (PJ_HAS_ERROR_STRING != 0)

    {
	const char *tmp = NULL;
	tmp = ERR_reason_error_string(ssl_err);
	if (tmp) {
	    pj_ansi_strncpy(buf, tmp, bufsize);
	    errstr = pj_str(buf);
	    return errstr;
	}
    }

#endif	/* PJ_HAS_ERROR_STRING */

    errstr.ptr = buf;
    errstr.slen = pj_ansi_snprintf(buf, bufsize, 
				   "Unknown OpenSSL error %lu",
				   ssl_err);
    if (errstr.slen < 1 || errstr.slen >= (int)bufsize)
	errstr.slen = bufsize - 1;
    return errstr;
}


/* OpenSSL library initialization counter */
static int openssl_init_count;

/* OpenSSL available ciphers */
static unsigned openssl_cipher_num;
static struct openssl_ciphers_t {
    pj_ssl_cipher    id;
    const char	    *name;
} openssl_ciphers[PJ_SSL_SOCK_MAX_CIPHERS];

/* OpenSSL available curves */
static unsigned openssl_curves_num;
static struct openssl_curves_t {
    pj_ssl_curve    id;
    const char	    *name;
} openssl_curves[PJ_SSL_SOCK_MAX_CURVES];

/* OpenSSL application data index */
static int sslsock_idx;


/* Initialize OpenSSL */
static pj_status_t init_openssl(void)
{
    pj_status_t status;

    if (openssl_init_count)
	return PJ_SUCCESS;

    openssl_init_count = 1;

    /* Register error subsystem */
    status = pj_register_strerror(PJ_SSL_ERRNO_START, 
				  PJ_SSL_ERRNO_SPACE_SIZE, 
				  &ssl_strerror);
    pj_assert(status == PJ_SUCCESS);

    /* Init OpenSSL lib */
#if USING_LIBRESSL || OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
    SSL_load_error_strings();
#else
    OPENSSL_init_ssl(0, NULL);
#endif
#if OPENSSL_VERSION_NUMBER < 0x009080ffL
    /* This is now synonym of SSL_library_init() */
    OpenSSL_add_all_algorithms();
#endif

    /* Init available ciphers */
    if (openssl_cipher_num == 0 || openssl_curves_num == 0) {
	SSL_METHOD *meth = NULL;
	SSL_CTX *ctx;
	SSL *ssl;
	STACK_OF(SSL_CIPHER) *sk_cipher;
	unsigned i, n;
	int nid;
	const char *cname;

#if (USING_LIBRESSL && LIBRESSL_VERSION_NUMBER < 0x2020100fL)\
    || OPENSSL_VERSION_NUMBER < 0x10100000L

	meth = (SSL_METHOD*)SSLv23_server_method();
	if (!meth)
	    meth = (SSL_METHOD*)TLSv1_server_method();
#ifndef OPENSSL_NO_SSL3_METHOD
	if (!meth)
	    meth = (SSL_METHOD*)SSLv3_server_method();
#endif
#ifndef OPENSSL_NO_SSL2
	if (!meth)
	    meth = (SSL_METHOD*)SSLv2_server_method();
#endif

#else
	/* Specific version methods are deprecated in 1.1.0 */
	meth = (SSL_METHOD*)TLS_method();
#endif

	pj_assert(meth);

	ctx=SSL_CTX_new(meth);
	SSL_CTX_set_cipher_list(ctx, "ALL:COMPLEMENTOFALL");

	ssl = SSL_new(ctx);

	sk_cipher = SSL_get_ciphers(ssl);

	n = sk_SSL_CIPHER_num(sk_cipher);
	if (n > PJ_ARRAY_SIZE(openssl_ciphers))
	    n = PJ_ARRAY_SIZE(openssl_ciphers);

	for (i = 0; i < n; ++i) {
	    const SSL_CIPHER *c;
	    c = sk_SSL_CIPHER_value(sk_cipher,i);
	    openssl_ciphers[i].id = (pj_ssl_cipher)
				    (pj_uint32_t)SSL_CIPHER_get_id(c) &
				    0x00FFFFFF;
	    openssl_ciphers[i].name = SSL_CIPHER_get_name(c);
	}
	openssl_cipher_num = n;

	SSL_set_session(ssl, SSL_SESSION_new());

#if !USING_LIBRESSL && !defined(OPENSSL_NO_EC) \
    && OPENSSL_VERSION_NUMBER >= 0x1000200fL
	openssl_curves_num = SSL_get_shared_curve(ssl,-1);
	if (openssl_curves_num > PJ_ARRAY_SIZE(openssl_curves))
	    openssl_curves_num = PJ_ARRAY_SIZE(openssl_curves);

	for (i = 0; i < openssl_curves_num; i++) {
	    nid = SSL_get_shared_curve(ssl, i);

	    if (nid & TLSEXT_nid_unknown) {
		cname = "curve unknown";
		nid &= 0xFFFF;
	    } else {
		cname = EC_curve_nid2nist(nid);
		if (!cname)
		    cname = OBJ_nid2sn(nid);
	    }

	    openssl_curves[i].id   = get_cid_from_nid(nid);
	    openssl_curves[i].name = cname;
	}
#else
	PJ_UNUSED_ARG(nid);
	PJ_UNUSED_ARG(cname);
	openssl_curves_num = 0;
#endif

	SSL_free(ssl);
	SSL_CTX_free(ctx);
    }

    /* Create OpenSSL application data index for SSL socket */
    sslsock_idx = SSL_get_ex_new_index(0, "SSL socket", NULL, NULL, NULL);

    return status;
}


/* Shutdown OpenSSL */
static void shutdown_openssl(void)
{
    PJ_UNUSED_ARG(openssl_init_count);
}


/* SSL password callback. */
static int password_cb(char *buf, int num, int rwflag, void *user_data)
{
    pj_ssl_cert_t *cert = (pj_ssl_cert_t*) user_data;

    PJ_UNUSED_ARG(rwflag);

    if(num < cert->privkey_pass.slen)
	return 0;
    
    pj_memcpy(buf, cert->privkey_pass.ptr, cert->privkey_pass.slen);
    return (int)cert->privkey_pass.slen;
}


/* SSL password callback. */
static int verify_cb(int preverify_ok, X509_STORE_CTX *x509_ctx)
{
    pj_ssl_sock_t *ssock;
    SSL *ossl_ssl;
    int err;

    /* Get SSL instance */
    ossl_ssl = X509_STORE_CTX_get_ex_data(x509_ctx, 
				    SSL_get_ex_data_X509_STORE_CTX_idx());
    pj_assert(ossl_ssl);

    /* Get SSL socket instance */
    ssock = SSL_get_ex_data(ossl_ssl, sslsock_idx);
    pj_assert(ssock);

    /* Store verification status */
    err = X509_STORE_CTX_get_error(x509_ctx);
    switch (err) {
    case X509_V_OK:
	break;

    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
	ssock->verify_status |= PJ_SSL_CERT_EISSUER_NOT_FOUND;
	break;

    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
    case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
    case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
	ssock->verify_status |= PJ_SSL_CERT_EINVALID_FORMAT;
	break;

    case X509_V_ERR_CERT_NOT_YET_VALID:
    case X509_V_ERR_CERT_HAS_EXPIRED:
	ssock->verify_status |= PJ_SSL_CERT_EVALIDITY_PERIOD;
	break;

    case X509_V_ERR_UNABLE_TO_GET_CRL:
    case X509_V_ERR_CRL_NOT_YET_VALID:
    case X509_V_ERR_CRL_HAS_EXPIRED:
    case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
    case X509_V_ERR_CRL_SIGNATURE_FAILURE:
    case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
    case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
	ssock->verify_status |= PJ_SSL_CERT_ECRL_FAILURE;
	break;	

    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
    case X509_V_ERR_CERT_UNTRUSTED:
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
	ssock->verify_status |= PJ_SSL_CERT_EUNTRUSTED;
	break;	

    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
    case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
    case X509_V_ERR_AKID_SKID_MISMATCH:
    case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
    case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
	ssock->verify_status |= PJ_SSL_CERT_EISSUER_MISMATCH;
	break;

    case X509_V_ERR_CERT_REVOKED:
	ssock->verify_status |= PJ_SSL_CERT_EREVOKED;
	break;	

    case X509_V_ERR_INVALID_PURPOSE:
    case X509_V_ERR_CERT_REJECTED:
    case X509_V_ERR_INVALID_CA:
	ssock->verify_status |= PJ_SSL_CERT_EINVALID_PURPOSE;
	break;

    case X509_V_ERR_CERT_CHAIN_TOO_LONG: /* not really used */
    case X509_V_ERR_PATH_LENGTH_EXCEEDED:
	ssock->verify_status |= PJ_SSL_CERT_ECHAIN_TOO_LONG;
	break;

    /* Unknown errors */
    case X509_V_ERR_OUT_OF_MEM:
    default:
	ssock->verify_status |= PJ_SSL_CERT_EUNKNOWN;
	break;
    }

    /* When verification is not requested just return ok here, however
     * application can still get the verification status.
     */
    if (PJ_FALSE == ssock->param.verify_peer)
	preverify_ok = 1;

    return preverify_ok;
}

/* Setting SSL sock cipher list */
static pj_status_t set_cipher_list(pj_ssl_sock_t *ssock);
/* Setting SSL sock curves list */
static pj_status_t set_curves_list(pj_ssl_sock_t *ssock);
/* Setting sigalgs list */
static pj_status_t set_sigalgs(pj_ssl_sock_t *ssock);
/* Setting entropy for rng */
static void set_entropy(pj_ssl_sock_t *ssock);

/* Create and initialize new SSL context and instance */
static pj_status_t create_ssl(pj_ssl_sock_t *ssock)
{
#if !defined(OPENSSL_NO_DH)
    BIO *bio;
    DH *dh;
    long options;
#endif
    SSL_METHOD *ssl_method = NULL;
    SSL_CTX *ctx;
    pj_uint32_t ssl_opt = 0;
    pj_ssl_cert_t *cert;
    int mode, rc;
    pj_status_t status;
        
    pj_assert(ssock);

    cert = ssock->cert;

    /* Make sure OpenSSL library has been initialized */
    init_openssl();

    set_entropy(ssock);

    if (ssock->param.proto == PJ_SSL_SOCK_PROTO_DEFAULT)
	ssock->param.proto = PJ_SSL_SOCK_PROTO_SSL23;

    /* Determine SSL method to use */
#if (USING_LIBRESSL && LIBRESSL_VERSION_NUMBER < 0x2020100fL)\
    || OPENSSL_VERSION_NUMBER < 0x10100000L
    switch (ssock->param.proto) {
    case PJ_SSL_SOCK_PROTO_TLS1:
	ssl_method = (SSL_METHOD*)TLSv1_method();
	break;
#ifndef OPENSSL_NO_SSL2
    case PJ_SSL_SOCK_PROTO_SSL2:
	ssl_method = (SSL_METHOD*)SSLv2_method();
	break;
#endif
#ifndef OPENSSL_NO_SSL3_METHOD
    case PJ_SSL_SOCK_PROTO_SSL3:
	ssl_method = (SSL_METHOD*)SSLv3_method();
#endif
	break;
    }
#else
    /* Specific version methods are deprecated in 1.1.0 */
    ssl_method = (SSL_METHOD*)TLS_method();
#endif

    if (!ssl_method) {
	ssl_method = (SSL_METHOD*)SSLv23_method();

#ifdef SSL_OP_NO_SSLv2
	/** Check if SSLv2 is enabled */
	ssl_opt |= ((ssock->param.proto & PJ_SSL_SOCK_PROTO_SSL2)==0)?
		    SSL_OP_NO_SSLv2:0;
#endif

#ifdef SSL_OP_NO_SSLv3
	/** Check if SSLv3 is enabled */
	ssl_opt |= ((ssock->param.proto & PJ_SSL_SOCK_PROTO_SSL3)==0)?
		    SSL_OP_NO_SSLv3:0;
#endif

#ifdef SSL_OP_NO_TLSv1
	/** Check if TLSv1 is enabled */
	ssl_opt |= ((ssock->param.proto & PJ_SSL_SOCK_PROTO_TLS1)==0)?
		    SSL_OP_NO_TLSv1:0;
#endif

#ifdef SSL_OP_NO_TLSv1_1
	/** Check if TLSv1_1 is enabled */
	ssl_opt |= ((ssock->param.proto & PJ_SSL_SOCK_PROTO_TLS1_1)==0)?
		    SSL_OP_NO_TLSv1_1:0;
#endif

#ifdef SSL_OP_NO_TLSv1_2
	/** Check if TLSv1_2 is enabled */
	ssl_opt |= ((ssock->param.proto & PJ_SSL_SOCK_PROTO_TLS1_2)==0)?
		    SSL_OP_NO_TLSv1_2:0;

#endif

    }

    /* Create SSL context */
    ctx = SSL_CTX_new(ssl_method);
    if (ctx == NULL) {
	return GET_SSL_STATUS(ssock);
    }
    if (ssl_opt)
	SSL_CTX_set_options(ctx, ssl_opt);

    /* Apply credentials */
    if (cert) {
	/* Load CA list if one is specified. */
	if (cert->CA_file.slen || cert->CA_path.slen) {

	    rc = SSL_CTX_load_verify_locations(
			ctx,
			cert->CA_file.slen == 0 ? NULL : cert->CA_file.ptr,
			cert->CA_path.slen == 0 ? NULL : cert->CA_path.ptr);

	    if (rc != 1) {
		status = GET_SSL_STATUS(ssock);
		if (cert->CA_file.slen) {
		    PJ_LOG(1,(ssock->pool->obj_name,
			      "Error loading CA list file '%s'",
			      cert->CA_file.ptr));
		}
		if (cert->CA_path.slen) {
		    PJ_LOG(1,(ssock->pool->obj_name,
			      "Error loading CA path '%s'",
			      cert->CA_path.ptr));
		}
		SSL_CTX_free(ctx);
		return status;
	    }
	}
    
	/* Set password callback */
	if (cert->privkey_pass.slen) {
	    SSL_CTX_set_default_passwd_cb(ctx, password_cb);
	    SSL_CTX_set_default_passwd_cb_userdata(ctx, cert);
	}


	/* Load certificate if one is specified */
	if (cert->cert_file.slen) {

	    /* Load certificate chain from file into ctx */
	    rc = SSL_CTX_use_certificate_chain_file(ctx, cert->cert_file.ptr);

	    if(rc != 1) {
		status = GET_SSL_STATUS(ssock);
		PJ_LOG(1,(ssock->pool->obj_name, "Error loading certificate "
			  "chain file '%s'", cert->cert_file.ptr));
		SSL_CTX_free(ctx);
		return status;
	    }
	}


	/* Load private key if one is specified */
	if (cert->privkey_file.slen) {
	    /* Adds the first private key found in file to ctx */
	    rc = SSL_CTX_use_PrivateKey_file(ctx, cert->privkey_file.ptr, 
					     SSL_FILETYPE_PEM);

	    if(rc != 1) {
		status = GET_SSL_STATUS(ssock);
		PJ_LOG(1,(ssock->pool->obj_name, "Error adding private key "
			  "from '%s'", cert->privkey_file.ptr));
		SSL_CTX_free(ctx);
		return status;
	    }

#if !defined(OPENSSL_NO_DH)
	    if (ssock->is_server) {
		bio = BIO_new_file(cert->privkey_file.ptr, "r");
		if (bio != NULL) {
		    dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
		    if (dh != NULL) {
			if (SSL_CTX_set_tmp_dh(ctx, dh)) {
			    options = SSL_OP_CIPHER_SERVER_PREFERENCE |
    #if !defined(OPENSSL_NO_ECDH) && OPENSSL_VERSION_NUMBER >= 0x10000000L
				      SSL_OP_SINGLE_ECDH_USE |
    #endif
				      SSL_OP_SINGLE_DH_USE;
			    options = SSL_CTX_set_options(ctx, options);
			    PJ_LOG(4,(ssock->pool->obj_name, "SSL DH "
				     "initialized, PFS cipher-suites enabled"));
			}
			DH_free(dh);
		    }
		    BIO_free(bio);
		}
	    }
#endif
	}

	/* Load from buffer. */
	if (cert->cert_buf.slen) {
	    BIO *cbio;
	    X509 *xcert = NULL;
	    
	    cbio = BIO_new_mem_buf((void*)cert->cert_buf.ptr,
				   cert->cert_buf.slen);
	    if (cbio != NULL) {
		xcert = PEM_read_bio_X509(cbio, NULL, 0, NULL);
		if (xcert != NULL) {
		    rc = SSL_CTX_use_certificate(ctx, xcert);
		    if (rc != 1) {
			status = GET_SSL_STATUS(ssock);
			PJ_LOG(1, (ssock->pool->obj_name, "Error loading "
				   "chain certificate from buffer"));
			X509_free(xcert);
			BIO_free(cbio);
			SSL_CTX_free(ctx);
			return status;
		    }
		    X509_free(xcert);
		}
		BIO_free(cbio);
	    }	    
	}

	if (cert->CA_buf.slen) {
	    BIO *cbio = BIO_new_mem_buf((void*)cert->CA_buf.ptr,
					cert->CA_buf.slen);
	    X509_STORE *cts = SSL_CTX_get_cert_store(ctx);

	    if (cbio && cts) {
		STACK_OF(X509_INFO) *inf = PEM_X509_INFO_read_bio(cbio, NULL, 
								  NULL, NULL);

		if (inf != NULL) {
		    int i = 0;		    
		    for (; i < sk_X509_INFO_num(inf); i++) {
			X509_INFO *itmp = sk_X509_INFO_value(inf, i);
			if (itmp->x509) {
			    X509_STORE_add_cert(cts, itmp->x509);
			}
		    }
		}
		sk_X509_INFO_pop_free(inf, X509_INFO_free);
		BIO_free(cbio);
	    }
	}

	if (cert->privkey_buf.slen) {
	    BIO *kbio;	    
	    EVP_PKEY *pkey = NULL;

	    kbio = BIO_new_mem_buf((void*)cert->privkey_buf.ptr,
				   cert->privkey_buf.slen);
	    if (kbio != NULL) {
		pkey = PEM_read_bio_PrivateKey(kbio, NULL, 0, NULL);
		if (pkey) {
		    rc = SSL_CTX_use_PrivateKey(ctx, pkey);
		    if (rc != 1) {
			status = GET_SSL_STATUS(ssock);
			PJ_LOG(1, (ssock->pool->obj_name, "Error adding "
				   "private key from buffer"));
			EVP_PKEY_free(pkey);
			BIO_free(kbio);
			SSL_CTX_free(ctx);
			return status;
		    }
		    EVP_PKEY_free(pkey);
		}
		if (ssock->is_server) {
		    dh = PEM_read_bio_DHparams(kbio, NULL, NULL, NULL);
		    if (dh != NULL) {
			if (SSL_CTX_set_tmp_dh(ctx, dh)) {
			    options = SSL_OP_CIPHER_SERVER_PREFERENCE |
    #if !defined(OPENSSL_NO_ECDH) && OPENSSL_VERSION_NUMBER >= 0x10000000L
				      SSL_OP_SINGLE_ECDH_USE |
    #endif
				      SSL_OP_SINGLE_DH_USE;
			    options = SSL_CTX_set_options(ctx, options);
			    PJ_LOG(4,(ssock->pool->obj_name, "SSL DH "
				     "initialized, PFS cipher-suites enabled"));
			}
			DH_free(dh);
		    }
		}
		BIO_free(kbio);
	    }	    
	}
    }

    if (ssock->is_server) {
	char *p = NULL;

	/* If certificate file name contains "_rsa.", let's check if there are
	 * ecc and dsa certificates too.
	 */
	if (cert && cert->cert_file.slen) {
	    const pj_str_t RSA = {"_rsa.", 5};
	    p = pj_strstr(&cert->cert_file, &RSA);
	    if (p) p++; /* Skip underscore */
	}
	if (p) {
	    /* Certificate type string length must be exactly 3 */
	    enum { CERT_TYPE_LEN = 3 };
	    const char* cert_types[] = { "ecc", "dsa" };
	    char *cf = cert->cert_file.ptr;
	    int i;

	    /* Check and load ECC & DSA certificates & private keys */
	    for (i = 0; i < PJ_ARRAY_SIZE(cert_types); ++i) {
		int err;

		pj_memcpy(p, cert_types[i], CERT_TYPE_LEN);
		if (!pj_file_exists(cf))
		    continue;

		err = SSL_CTX_use_certificate_chain_file(ctx, cf);
		if (err == 1)
		    err = SSL_CTX_use_PrivateKey_file(ctx, cf,
						      SSL_FILETYPE_PEM);
		if (err == 1) {
		    PJ_LOG(4,(ssock->pool->obj_name,
			      "Additional certificate '%s' loaded.", cf));
		} else {
		    pj_perror(1, ssock->pool->obj_name, GET_SSL_STATUS(ssock),
			      "Error loading certificate file '%s'", cf);
		    ERR_clear_error();
		}
	    }

	    /* Put back original name */
	    pj_memcpy(p, "rsa", CERT_TYPE_LEN);
	}

    #ifndef SSL_CTRL_SET_ECDH_AUTO
	#define SSL_CTRL_SET_ECDH_AUTO 94
    #endif

	/* SSL_CTX_set_ecdh_auto(ctx,on) requires OpenSSL 1.0.2 which wraps: */
	if (SSL_CTX_ctrl(ctx, SSL_CTRL_SET_ECDH_AUTO, 1, NULL)) {
	    PJ_LOG(4,(ssock->pool->obj_name, "SSL ECDH initialized "
		      "(automatic), faster PFS ciphers enabled"));
    #if !defined(OPENSSL_NO_ECDH) && OPENSSL_VERSION_NUMBER >= 0x10000000L && \
	OPENSSL_VERSION_NUMBER < 0x10100000L
	} else {
	    /* enables AES-128 ciphers, to get AES-256 use NID_secp384r1 */
	    EC_KEY *ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
	    if (ecdh != NULL) {
		if (SSL_CTX_set_tmp_ecdh(ctx, ecdh)) {
		    PJ_LOG(4,(ssock->pool->obj_name, "SSL ECDH initialized "
			      "(secp256r1), faster PFS cipher-suites enabled"));
		}
		EC_KEY_free(ecdh);
	    }
    #endif
	}
    } else {
	X509_STORE *pkix_validation_store = SSL_CTX_get_cert_store(ctx);
	if (NULL != pkix_validation_store) {
#if defined(X509_V_FLAG_TRUSTED_FIRST)
	    X509_STORE_set_flags(pkix_validation_store, 
				 X509_V_FLAG_TRUSTED_FIRST);
#endif
#if defined(X509_V_FLAG_PARTIAL_CHAIN)
	    X509_STORE_set_flags(pkix_validation_store, 
				 X509_V_FLAG_PARTIAL_CHAIN);
#endif
	}
    }

    /* Create SSL instance */
    ssock->ossl_ctx = ctx;
    ssock->ossl_ssl = SSL_new(ssock->ossl_ctx);
    if (ssock->ossl_ssl == NULL) {
	return GET_SSL_STATUS(ssock);
    }

    /* Set SSL sock as application data of SSL instance */
    SSL_set_ex_data(ssock->ossl_ssl, sslsock_idx, ssock);

    /* SSL verification options */
    mode = SSL_VERIFY_PEER;
    if (ssock->is_server && ssock->param.require_client_cert)
	mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;

    SSL_set_verify(ssock->ossl_ssl, mode, &verify_cb);

    /* Set cipher list */
    status = set_cipher_list(ssock);
    if (status != PJ_SUCCESS)
	return status;

    /* Set curve list */
    status = set_curves_list(ssock);
    if (status != PJ_SUCCESS)
	return status;

    /* Set sigalg list */
    status = set_sigalgs(ssock);
    if (status != PJ_SUCCESS)
	return status;

    /* Setup SSL BIOs */
    ssock->ossl_rbio = BIO_new(BIO_s_mem());
    ssock->ossl_wbio = BIO_new(BIO_s_mem());
    (void)BIO_set_close(ssock->ossl_rbio, BIO_CLOSE);
    (void)BIO_set_close(ssock->ossl_wbio, BIO_CLOSE);
    SSL_set_bio(ssock->ossl_ssl, ssock->ossl_rbio, ssock->ossl_wbio);

    return PJ_SUCCESS;
}


/* Destroy SSL context and instance */
static void destroy_ssl(pj_ssl_sock_t *ssock)
{
    /* Destroy SSL instance */
    if (ssock->ossl_ssl) {
	/**
	 * Avoid calling SSL_shutdown() if handshake wasn't completed.
	 * OpenSSL 1.0.2f complains if SSL_shutdown() is called during an
	 * SSL handshake, while previous versions always return 0.	 
	 */
	if (SSL_in_init(ssock->ossl_ssl) == 0) {
	    SSL_shutdown(ssock->ossl_ssl);
	}   	
	SSL_free(ssock->ossl_ssl); /* this will also close BIOs */
	ssock->ossl_ssl = NULL;
    }

    /* Destroy SSL context */
    if (ssock->ossl_ctx) {
	SSL_CTX_free(ssock->ossl_ctx);
	ssock->ossl_ctx = NULL;
    }

    /* Potentially shutdown OpenSSL library if this is the last
     * context exists.
     */
    shutdown_openssl();
}


/* Close sockets */
static void close_sockets(pj_ssl_sock_t *ssock)
{
    pj_activesock_t *asock;
    pj_sock_t sock;

    /* This can happen when pj_ssl_sock_create() fails. */
    if (!ssock->write_mutex)
    	return;

    pj_lock_acquire(ssock->write_mutex);
    asock = ssock->asock;
    if (asock) {
        // Don't set ssock->asock to NULL, as it may trigger assertion in
        // send operation. This should be safe as active socket will simply
        // return PJ_EINVALIDOP on any operation if it is already closed.
        //ssock->asock = NULL;
        ssock->sock = PJ_INVALID_SOCKET;
    }
    sock = ssock->sock;
    if (sock != PJ_INVALID_SOCKET)
        ssock->sock = PJ_INVALID_SOCKET;
    pj_lock_release(ssock->write_mutex);

    if (asock)
        pj_activesock_close(asock);

    if (sock != PJ_INVALID_SOCKET)
        pj_sock_close(sock);
}


/* Reset SSL socket state */
static void reset_ssl_sock_state(pj_ssl_sock_t *ssock)
{
    pj_lock_acquire(ssock->write_mutex);
    ssock->ssl_state = SSL_STATE_NULL;
    pj_lock_release(ssock->write_mutex);

    close_sockets(ssock);

    /* Upon error, OpenSSL may leave any error description in the thread 
     * error queue, which sometime may cause next call to SSL API returning
     * false error alarm, e.g: in Linux, SSL_CTX_use_certificate_chain_file()
     * returning false error after a handshake error (in different SSL_CTX!).
     * For now, just clear thread error queue here.
     */
    ERR_clear_error();
}


/* Generate cipher list with user preference order in OpenSSL format */
static pj_status_t set_cipher_list(pj_ssl_sock_t *ssock)
{
    pj_pool_t *tmp_pool = NULL;
    char *buf = NULL;
    enum { BUF_SIZE = 8192 };
    pj_str_t cipher_list;
    STACK_OF(SSL_CIPHER) *sk_cipher;
    unsigned i;
    int j, ret;

    if (ssock->param.ciphers_num == 0) {
	ret = SSL_set_cipher_list(ssock->ossl_ssl, PJ_SSL_SOCK_OSSL_CIPHERS);
    	if (ret < 1) {
	    return GET_SSL_STATUS(ssock);
    	}    
	
	return PJ_SUCCESS;
    }

    /* Create temporary pool. */
    tmp_pool = pj_pool_create(ssock->pool->factory, "ciphpool", BUF_SIZE, 
			      BUF_SIZE/2 , NULL);
    if (!tmp_pool)
	return PJ_ENOMEM;

    buf = (char *)pj_pool_zalloc(tmp_pool, BUF_SIZE);

    pj_strset(&cipher_list, buf, 0);

    /* Set SSL with ALL available ciphers */
    SSL_set_cipher_list(ssock->ossl_ssl, "ALL:COMPLEMENTOFALL");

    /* Generate user specified cipher list in OpenSSL format */
    sk_cipher = SSL_get_ciphers(ssock->ossl_ssl);
    for (i = 0; i < ssock->param.ciphers_num; ++i) {
	for (j = 0; j < sk_SSL_CIPHER_num(sk_cipher); ++j) {
	    const SSL_CIPHER *c;
	    c = sk_SSL_CIPHER_value(sk_cipher, j);
	    if (ssock->param.ciphers[i] == (pj_ssl_cipher)
					   ((pj_uint32_t)SSL_CIPHER_get_id(c) &
					   0x00FFFFFF))
	    {
		const char *c_name;

		c_name = SSL_CIPHER_get_name(c);

		/* Check buffer size */
		if (cipher_list.slen + pj_ansi_strlen(c_name) + 2 >
		    BUF_SIZE)
		{
		    pj_assert(!"Insufficient temporary buffer for cipher");
		    return PJ_ETOOMANY;
		}

		/* Add colon separator */
		if (cipher_list.slen)
		    pj_strcat2(&cipher_list, ":");

		/* Add the cipher */
		pj_strcat2(&cipher_list, c_name);
		break;
	    }
	}	
    }

    /* Put NULL termination in the generated cipher list */
    cipher_list.ptr[cipher_list.slen] = '\0';

    /* Finally, set chosen cipher list */
    ret = SSL_set_cipher_list(ssock->ossl_ssl, buf);
    if (ret < 1) {
	pj_pool_release(tmp_pool);
	return GET_SSL_STATUS(ssock);
    }

    pj_pool_release(tmp_pool);
    return PJ_SUCCESS;
}

static pj_status_t set_curves_list(pj_ssl_sock_t *ssock)
{
#if !USING_LIBRESSL && !defined(OPENSSL_NO_EC) \
    && OPENSSL_VERSION_NUMBER >= 0x1000200fL
    int ret;
    int curves[PJ_SSL_SOCK_MAX_CURVES];
    unsigned cnt;

    if (ssock->param.curves_num == 0)
	return PJ_SUCCESS;

    for (cnt = 0; cnt < ssock->param.curves_num; cnt++) {
	curves[cnt] = get_nid_from_cid(ssock->param.curves[cnt]);
    }

    if( SSL_is_server(ssock->ossl_ssl) ) {
	ret = SSL_set1_curves(ssock->ossl_ssl, curves,
			      ssock->param.curves_num);
	if (ret < 1)
	    return GET_SSL_STATUS(ssock);
    } else {
	ret = SSL_CTX_set1_curves(ssock->ossl_ctx, curves,
				  ssock->param.curves_num);
	if (ret < 1)
	    return GET_SSL_STATUS(ssock);
    }
#else
    PJ_UNUSED_ARG(ssock);
#endif
    return PJ_SUCCESS;
}

static pj_status_t set_sigalgs(pj_ssl_sock_t *ssock)
{
#if !USING_LIBRESSL && OPENSSL_VERSION_NUMBER >= 0x1000200fL
    int ret;

    if (ssock->param.sigalgs.ptr && ssock->param.sigalgs.slen) {
	if (ssock->is_server) {
	    ret = SSL_set1_client_sigalgs_list(ssock->ossl_ssl,
	    				       ssock->param.sigalgs.ptr);
	} else {
	    ret = SSL_set1_sigalgs_list(ssock->ossl_ssl,
	    				ssock->param.sigalgs.ptr);
	}

	if (ret < 1)
	    return GET_SSL_STATUS(ssock);
    }
#else
    PJ_UNUSED_ARG(ssock);
#endif
    return PJ_SUCCESS;
}

static void set_entropy(pj_ssl_sock_t *ssock)
{
    int ret;

    switch (ssock->param.entropy_type) {
#ifndef OPENSSL_NO_EGD
	case PJ_SSL_ENTROPY_EGD:
	    ret = RAND_egd(ssock->param.entropy_path.ptr);
	    break;
#endif
	case PJ_SSL_ENTROPY_RANDOM:
	    ret = RAND_load_file("/dev/random",255);
	    break;
	case PJ_SSL_ENTROPY_URANDOM:
	    ret = RAND_load_file("/dev/urandom",255);
	    break;
	case PJ_SSL_ENTROPY_FILE:
	    ret = RAND_load_file(ssock->param.entropy_path.ptr,255);
	    break;
	case PJ_SSL_ENTROPY_NONE:
	    default:
	    return;
	    break;
    }

    if (ret < 0) {
	PJ_LOG(3, (ssock->pool->obj_name, "SSL failed to reseed with "
					  "entropy type %d",
			  		  ssock->param.entropy_type));
    }
}

/* Parse OpenSSL ASN1_TIME to pj_time_val and GMT info */
static pj_bool_t parse_ossl_asn1_time(pj_time_val *tv, pj_bool_t *gmt,
				      const ASN1_TIME *tm)
{
    unsigned long parts[7] = {0};
    char *p, *end;
    unsigned len;
    pj_bool_t utc;
    pj_parsed_time pt;
    int i;

    utc = tm->type == V_ASN1_UTCTIME;
    p = (char*)tm->data;
    len = tm->length;
    end = p + len - 1;

    /* GMT */
    *gmt = (*end == 'Z');

    /* parse parts */
    for (i = 0; i < 7 && p < end; ++i) {
	pj_str_t st;

	if (i==0 && !utc) {
	    /* 4 digits year part for non-UTC time format */
	    st.slen = 4;
	} else if (i==6) {
	    /* fraction of seconds */
	    if (*p == '.') ++p;
	    st.slen = end - p + 1;
	} else {
	    /* other parts always 2 digits length */
	    st.slen = 2;
	}
	st.ptr = p;

	parts[i] = pj_strtoul(&st);
	p += st.slen;
    }

    /* encode parts to pj_time_val */
    pt.year = parts[0];
    if (utc)
	pt.year += (pt.year < 50)? 2000:1900;
    pt.mon = parts[1] - 1;
    pt.day = parts[2];
    pt.hour = parts[3];
    pt.min = parts[4];
    pt.sec = parts[5];
    pt.msec = parts[6];

    pj_time_encode(&pt, tv);

    return PJ_TRUE;
}


/* Get Common Name field string from a general name string */
static void get_cn_from_gen_name(const pj_str_t *gen_name, pj_str_t *cn)
{
    pj_str_t CN_sign = {"/CN=", 4};
    char *p, *q;

    pj_bzero(cn, sizeof(pj_str_t));

    p = pj_strstr(gen_name, &CN_sign);
    if (!p)
	return;

    p += 4; /* shift pointer to value part */
    pj_strset(cn, p, gen_name->slen - (p - gen_name->ptr));
    q = pj_strchr(cn, '/');
    if (q)
	cn->slen = q - p;
}


/* Get certificate info from OpenSSL X509, in case the certificate info
 * hal already populated, this function will check if the contents need 
 * to be updated by inspecting the issuer and the serial number.
 */
static void get_cert_info(pj_pool_t *pool, pj_ssl_cert_info *ci, X509 *x,
			  pj_bool_t get_pem)
{
    pj_bool_t update_needed;
    char buf[512];
    pj_uint8_t serial_no[64] = {0}; /* should be >= sizeof(ci->serial_no) */
    const pj_uint8_t *q;
    unsigned len;
    GENERAL_NAMES *names = NULL;

    pj_assert(pool && ci && x);

    /* Get issuer */
    X509_NAME_oneline(X509_get_issuer_name(x), buf, sizeof(buf));

    /* Get serial no */
    q = (const pj_uint8_t*) M_ASN1_STRING_data(X509_get_serialNumber(x));
    len = M_ASN1_STRING_length(X509_get_serialNumber(x));
    if (len > sizeof(ci->serial_no)) 
	len = sizeof(ci->serial_no);
    pj_memcpy(serial_no + sizeof(ci->serial_no) - len, q, len);

    /* Check if the contents need to be updated. */
    update_needed = pj_strcmp2(&ci->issuer.info, buf) || 
	            pj_memcmp(ci->serial_no, serial_no, sizeof(ci->serial_no));
    if (!update_needed)
	return;

    /* Update cert info */

    pj_bzero(ci, sizeof(pj_ssl_cert_info));

    /* Version */
    ci->version = X509_get_version(x) + 1;

    /* Issuer */
    pj_strdup2(pool, &ci->issuer.info, buf);
    get_cn_from_gen_name(&ci->issuer.info, &ci->issuer.cn);

    /* Serial number */
    pj_memcpy(ci->serial_no, serial_no, sizeof(ci->serial_no));

    /* Subject */
    pj_strdup2(pool, &ci->subject.info, 
	       X509_NAME_oneline(X509_get_subject_name(x),
				 buf, sizeof(buf)));
    get_cn_from_gen_name(&ci->subject.info, &ci->subject.cn);

    /* Validity */
    parse_ossl_asn1_time(&ci->validity.start, &ci->validity.gmt,
			 X509_get_notBefore(x));
    parse_ossl_asn1_time(&ci->validity.end, &ci->validity.gmt,
			 X509_get_notAfter(x));

    /* Subject Alternative Name extension */
    if (ci->version >= 3) {
	names = (GENERAL_NAMES*) X509_get_ext_d2i(x, NID_subject_alt_name,
						  NULL, NULL);
    }
    if (names) {
        unsigned i, cnt;

        cnt = sk_GENERAL_NAME_num(names);
	ci->subj_alt_name.entry = pj_pool_calloc(pool, cnt, 
					    sizeof(*ci->subj_alt_name.entry));

        for (i = 0; i < cnt; ++i) {
	    unsigned char *p = 0;
	    pj_ssl_cert_name_type type = PJ_SSL_CERT_NAME_UNKNOWN;
            const GENERAL_NAME *name;
	    
	    name = sk_GENERAL_NAME_value(names, i);

            switch (name->type) {
                case GEN_EMAIL:
                    len = ASN1_STRING_to_UTF8(&p, name->d.ia5);
		    type = PJ_SSL_CERT_NAME_RFC822;
                    break;
                case GEN_DNS:
                    len = ASN1_STRING_to_UTF8(&p, name->d.ia5);
		    type = PJ_SSL_CERT_NAME_DNS;
                    break;
                case GEN_URI:
                    len = ASN1_STRING_to_UTF8(&p, name->d.ia5);
		    type = PJ_SSL_CERT_NAME_URI;
                    break;
                case GEN_IPADD:
		    p = (unsigned char*)M_ASN1_STRING_data(name->d.ip);
		    len = M_ASN1_STRING_length(name->d.ip);
		    type = PJ_SSL_CERT_NAME_IP;
                    break;
		default:
		    break;
            }

	    if (p && len && type != PJ_SSL_CERT_NAME_UNKNOWN) {
		ci->subj_alt_name.entry[ci->subj_alt_name.cnt].type = type;
		if (type == PJ_SSL_CERT_NAME_IP) {
		    int af = pj_AF_INET();
		    if (len == sizeof(pj_in6_addr)) af = pj_AF_INET6();
		    pj_inet_ntop2(af, p, buf, sizeof(buf));
		    pj_strdup2(pool, 
		          &ci->subj_alt_name.entry[ci->subj_alt_name.cnt].name,
		          buf);
		} else {
		    pj_strdup2(pool, 
			  &ci->subj_alt_name.entry[ci->subj_alt_name.cnt].name, 
			  (char*)p);
		    OPENSSL_free(p);
		}
		ci->subj_alt_name.cnt++;
	    }
        }
    }

    if (get_pem) {
	/* Update raw Certificate info in PEM format. */
	BIO *bio;	
	BUF_MEM *ptr;
	
	bio = BIO_new(BIO_s_mem());
	if (!PEM_write_bio_X509(bio, x)) {
	    PJ_LOG(3,(THIS_FILE, "Error retrieving raw certificate info"));
	    ci->raw.ptr = NULL;
	    ci->raw.slen = 0;
	} else {
	    BIO_write(bio, "\0", 1);
	    BIO_get_mem_ptr(bio, &ptr);
	    pj_strdup2(pool, &ci->raw, ptr->data);	
	}	
	BIO_free(bio);	    
    }	 
}


/* Update local & remote certificates info. This function should be
 * called after handshake or renegotiation successfully completed.
 */
static void update_certs_info(pj_ssl_sock_t *ssock)
{
    X509 *x;

    pj_assert(ssock->ssl_state == SSL_STATE_ESTABLISHED);

    /* Active local certificate */
    x = SSL_get_certificate(ssock->ossl_ssl);
    if (x) {
	get_cert_info(ssock->pool, &ssock->local_cert_info, x, PJ_FALSE);
	/* Don't free local's X509! */
    } else {
	pj_bzero(&ssock->local_cert_info, sizeof(pj_ssl_cert_info));
    }

    /* Active remote certificate */
    x = SSL_get_peer_certificate(ssock->ossl_ssl);
    if (x) {
	get_cert_info(ssock->pool, &ssock->remote_cert_info, x, PJ_TRUE);
	/* Free peer's X509 */
	X509_free(x);
    } else {
	pj_bzero(&ssock->remote_cert_info, sizeof(pj_ssl_cert_info));
    }
}


/* When handshake completed:
 * - notify application
 * - if handshake failed, reset SSL state
 * - return PJ_FALSE when SSL socket instance is destroyed by application.
 */
static pj_bool_t on_handshake_complete(pj_ssl_sock_t *ssock, 
				       pj_status_t status)
{
    /* Cancel handshake timer */
    if (ssock->timer.id == TIMER_HANDSHAKE_TIMEOUT) {
	pj_timer_heap_cancel(ssock->param.timer_heap, &ssock->timer);
	ssock->timer.id = TIMER_NONE;
    }

    /* Update certificates info on successful handshake */
    if (status == PJ_SUCCESS)
	update_certs_info(ssock);

    /* Accepting */
    if (ssock->is_server) {
	if (status != PJ_SUCCESS) {
	    /* Handshake failed in accepting, destroy our self silently. */

	    char errmsg[PJ_ERR_MSG_SIZE];
	    char buf[PJ_INET6_ADDRSTRLEN+10];

	    pj_strerror(status, errmsg, sizeof(errmsg));
	    PJ_LOG(3,(ssock->pool->obj_name, "Handshake failed in accepting "
		      "%s: %s",
		      pj_sockaddr_print(&ssock->rem_addr, buf, sizeof(buf), 3),
		      errmsg));

	    if (*ssock->param.cb.on_accept_complete2) {
		(*ssock->param.cb.on_accept_complete2) 
		      (ssock->parent, ssock, (pj_sockaddr_t*)&ssock->rem_addr, 
		      pj_sockaddr_get_len((pj_sockaddr_t*)&ssock->rem_addr), 
		      status);
	    }

	    /* Originally, this is a workaround for ticket #985. However,
	     * a race condition may occur in multiple worker threads
	     * environment when we are destroying SSL objects while other
	     * threads are still accessing them.
	     * Please see ticket #1930 for more info.
	     */
#if 1 //(defined(PJ_WIN32) && PJ_WIN32!=0)||(defined(PJ_WIN64) && PJ_WIN64!=0)
	    if (ssock->param.timer_heap) {
		pj_time_val interval = {0, PJ_SSL_SOCK_DELAYED_CLOSE_TIMEOUT};

		ssock->ssl_state = SSL_STATE_NULL;
		close_sockets(ssock);

		if (ssock->timer.id != TIMER_NONE) {
		    pj_timer_heap_cancel(ssock->param.timer_heap,
					 &ssock->timer);
		}
		ssock->timer.id = TIMER_CLOSE;
		pj_time_val_normalize(&interval);
		if (pj_timer_heap_schedule(ssock->param.timer_heap, 
					   &ssock->timer, &interval) != 0)
		{
	    	    PJ_LOG(3,(ssock->pool->obj_name, "Failed to schedule "
		      	      "a delayed close. Race condition may occur."));
		    ssock->timer.id = TIMER_NONE;
		    pj_ssl_sock_close(ssock);
		}
	    } else {
		pj_ssl_sock_close(ssock);
	    }
#else
	    {
		pj_ssl_sock_close(ssock);
	    }
#endif

	    return PJ_FALSE;
	}
	/* Notify application the newly accepted SSL socket */
	if (ssock->param.cb.on_accept_complete2) {
	    pj_bool_t ret;
	    ret = (*ssock->param.cb.on_accept_complete2) 
		    (ssock->parent, ssock, (pj_sockaddr_t*)&ssock->rem_addr, 
		    pj_sockaddr_get_len((pj_sockaddr_t*)&ssock->rem_addr), 
		    status);
	    if (ret == PJ_FALSE)
		return PJ_FALSE;	
	} else if (ssock->param.cb.on_accept_complete) {
	    pj_bool_t ret;
	    ret = (*ssock->param.cb.on_accept_complete)
		      (ssock->parent, ssock, (pj_sockaddr_t*)&ssock->rem_addr,
		       pj_sockaddr_get_len((pj_sockaddr_t*)&ssock->rem_addr));
	    if (ret == PJ_FALSE)
		return PJ_FALSE;
	}
    }

    /* Connecting */
    else {
	/* On failure, reset SSL socket state first, as app may try to 
	 * reconnect in the callback.
	 */
	if (status != PJ_SUCCESS) {
	    /* Server disconnected us, possibly due to SSL nego failure */
	    if (status == PJ_EEOF) {
		unsigned long err;
		err = ERR_get_error();
		if (err != SSL_ERROR_NONE)
		    status = STATUS_FROM_SSL_ERR("connecting", ssock, err);
	    }
	    reset_ssl_sock_state(ssock);
	}
	if (ssock->param.cb.on_connect_complete) {
	    pj_bool_t ret;
	    ret = (*ssock->param.cb.on_connect_complete)(ssock, status);
	    if (ret == PJ_FALSE)
		return PJ_FALSE;
	}
    }

    return PJ_TRUE;
}

static write_data_t* alloc_send_data(pj_ssl_sock_t *ssock, pj_size_t len)
{
    send_buf_t *send_buf = &ssock->send_buf;
    pj_size_t avail_len, skipped_len = 0;
    char *reg1, *reg2;
    pj_size_t reg1_len, reg2_len;
    write_data_t *p;

    /* Check buffer availability */
    avail_len = send_buf->max_len - send_buf->len;
    if (avail_len < len)
	return NULL;

    /* If buffer empty, reset start pointer and return it */
    if (send_buf->len == 0) {
	send_buf->start = send_buf->buf;
	send_buf->len   = len;
	p = (write_data_t*)send_buf->start;
	goto init_send_data;
    }

    /* Free space may be wrapped/splitted into two regions, so let's
     * analyze them if any region can hold the write data.
     */
    reg1 = send_buf->start + send_buf->len;
    if (reg1 >= send_buf->buf + send_buf->max_len)
	reg1 -= send_buf->max_len;
    reg1_len = send_buf->max_len - send_buf->len;
    if (reg1 + reg1_len > send_buf->buf + send_buf->max_len) {
	reg1_len = send_buf->buf + send_buf->max_len - reg1;
	reg2 = send_buf->buf;
	reg2_len = send_buf->start - send_buf->buf;
    } else {
	reg2 = NULL;
	reg2_len = 0;
    }

    /* More buffer availability check, note that the write data must be in
     * a contigue buffer.
     */
    avail_len = PJ_MAX(reg1_len, reg2_len);
    if (avail_len < len)
	return NULL;

    /* Get the data slot */
    if (reg1_len >= len) {
	p = (write_data_t*)reg1;
    } else {
	p = (write_data_t*)reg2;
	skipped_len = reg1_len;
    }

    /* Update buffer length */
    send_buf->len += len + skipped_len;

init_send_data:
    /* Init the new send data */
    pj_bzero(p, sizeof(*p));
    pj_list_init(p);
    pj_list_push_back(&ssock->send_pending, p);

    return p;
}

static void free_send_data(pj_ssl_sock_t *ssock, write_data_t *wdata)
{
    send_buf_t *buf = &ssock->send_buf;
    write_data_t *spl = &ssock->send_pending;

    pj_assert(!pj_list_empty(&ssock->send_pending));
    
    /* Free slot from the buffer */
    if (spl->next == wdata && spl->prev == wdata) {
	/* This is the only data, reset the buffer */
	buf->start = buf->buf;
	buf->len = 0;
    } else if (spl->next == wdata) {
	/* This is the first data, shift start pointer of the buffer and
	 * adjust the buffer length.
	 */
	buf->start = (char*)wdata->next;
	if (wdata->next > wdata) {
	    buf->len -= ((char*)wdata->next - buf->start);
	} else {
	    /* Overlapped */
	    pj_size_t right_len, left_len;
	    right_len = buf->buf + buf->max_len - (char*)wdata;
	    left_len  = (char*)wdata->next - buf->buf;
	    buf->len -= (right_len + left_len);
	}
    } else if (spl->prev == wdata) {
	/* This is the last data, just adjust the buffer length */
	if (wdata->prev < wdata) {
	    pj_size_t jump_len;
	    jump_len = (char*)wdata -
		       ((char*)wdata->prev + wdata->prev->record_len);
	    buf->len -= (wdata->record_len + jump_len);
	} else {
	    /* Overlapped */
	    pj_size_t right_len, left_len;
	    right_len = buf->buf + buf->max_len -
			((char*)wdata->prev + wdata->prev->record_len);
	    left_len  = (char*)wdata + wdata->record_len - buf->buf;
	    buf->len -= (right_len + left_len);
	}
    }
    /* For data in the middle buffer, just do nothing on the buffer. The slot
     * will be freed later when freeing the first/last data.
     */
    
    /* Remove the data from send pending list */
    pj_list_erase(wdata);
}

#if 0
/* Just for testing send buffer alloc/free */
#include <pj/rand.h>
pj_status_t pj_ssl_sock_ossl_test_send_buf(pj_pool_t *pool)
{
    enum { MAX_CHUNK_NUM = 20 };
    unsigned chunk_size, chunk_cnt, i;
    write_data_t *wdata[MAX_CHUNK_NUM] = {0};
    pj_time_val now;
    pj_ssl_sock_t *ssock = NULL;
    pj_ssl_sock_param param;
    pj_status_t status;

    pj_gettimeofday(&now);
    pj_srand((unsigned)now.sec);

    pj_ssl_sock_param_default(&param);
    status = pj_ssl_sock_create(pool, &param, &ssock);
    if (status != PJ_SUCCESS) {
	return status;
    }

    if (ssock->send_buf.max_len == 0) {
	ssock->send_buf.buf = (char*)
			      pj_pool_alloc(ssock->pool, 
					    ssock->param.send_buffer_size);
	ssock->send_buf.max_len = ssock->param.send_buffer_size;
	ssock->send_buf.start = ssock->send_buf.buf;
	ssock->send_buf.len = 0;
    }

    chunk_size = ssock->param.send_buffer_size / MAX_CHUNK_NUM / 2;
    chunk_cnt = 0;
    for (i = 0; i < MAX_CHUNK_NUM; i++) {
	wdata[i] = alloc_send_data(ssock, pj_rand() % chunk_size + 321);
	if (wdata[i])
	    chunk_cnt++;
	else
	    break;
    }

    while (chunk_cnt) {
	i = pj_rand() % MAX_CHUNK_NUM;
	if (wdata[i]) {
	    free_send_data(ssock, wdata[i]);
	    wdata[i] = NULL;
	    chunk_cnt--;
	}
    }

    if (ssock->send_buf.len != 0)
	status = PJ_EBUG;

    pj_ssl_sock_close(ssock);
    return status;
}
#endif


/* Flush write BIO to network socket. Note that any access to write BIO
 * MUST be serialized, so mutex protection must cover any call to OpenSSL
 * API (that possibly generate data for write BIO) along with the call to
 * this function (flushing all data in write BIO generated by above 
 * OpenSSL API call).
 */
static pj_status_t flush_write_bio(pj_ssl_sock_t *ssock, 
				   pj_ioqueue_op_key_t *send_key,
				   pj_size_t orig_len,
				   unsigned flags)
{
    char *data;
    pj_ssize_t len;
    write_data_t *wdata;
    pj_size_t needed_len;
    pj_status_t status;

    pj_lock_acquire(ssock->write_mutex);

    /* Check if there is data in write BIO, flush it if any */
    if (!BIO_pending(ssock->ossl_wbio)) {
	pj_lock_release(ssock->write_mutex);
	return PJ_SUCCESS;
    }

    /* Get data and its length */
    len = BIO_get_mem_data(ssock->ossl_wbio, &data);
    if (len == 0) {
	pj_lock_release(ssock->write_mutex);
	return PJ_SUCCESS;
    }

    /* Calculate buffer size needed, and align it to 8 */
    needed_len = len + sizeof(write_data_t);
    needed_len = ((needed_len + 7) >> 3) << 3;

    /* Allocate buffer for send data */
    wdata = alloc_send_data(ssock, needed_len);
    if (wdata == NULL) {
	/* Oops, write BIO is ready but the send buffer is full, let's just
	 * queue it for sending and return PJ_EPENDING.
	 */
	ssock->send_buf_pending.data_len = needed_len;
	ssock->send_buf_pending.app_key = send_key;
	ssock->send_buf_pending.flags = flags;
	ssock->send_buf_pending.plain_data_len = orig_len;
	pj_lock_release(ssock->write_mutex);
	return PJ_EPENDING;
    }

    /* Copy the data and set its properties into the send data */
    pj_ioqueue_op_key_init(&wdata->key, sizeof(pj_ioqueue_op_key_t));
    wdata->key.user_data = wdata;
    wdata->app_key = send_key;
    wdata->record_len = needed_len;
    wdata->data_len = len;
    wdata->plain_data_len = orig_len;
    wdata->flags = flags;
    pj_memcpy(&wdata->data, data, len);

    /* Reset write BIO */
    (void)BIO_reset(ssock->ossl_wbio);

    /* Ticket #1573: Don't hold mutex while calling PJLIB socket send(). */
    pj_lock_release(ssock->write_mutex);

    /* Send it */
    if (ssock->param.sock_type == pj_SOCK_STREAM()) {
	status = pj_activesock_send(ssock->asock, &wdata->key, 
				    wdata->data.content, &len,
				    flags);
    } else {
	status = pj_activesock_sendto(ssock->asock, &wdata->key, 
				      wdata->data.content, &len,
				      flags,
				      (pj_sockaddr_t*)&ssock->rem_addr,
				      ssock->addr_len);
    }

    if (status != PJ_EPENDING) {
	/* When the sending is not pending, remove the wdata from send
	 * pending list.
	 */
	pj_lock_acquire(ssock->write_mutex);
	free_send_data(ssock, wdata);
	pj_lock_release(ssock->write_mutex);
    }

    return status;
}


static void on_timer(pj_timer_heap_t *th, struct pj_timer_entry *te)
{
    pj_ssl_sock_t *ssock = (pj_ssl_sock_t*)te->user_data;
    int timer_id = te->id;

    te->id = TIMER_NONE;

    PJ_UNUSED_ARG(th);

    switch (timer_id) {
    case TIMER_HANDSHAKE_TIMEOUT:
	PJ_LOG(1,(ssock->pool->obj_name, "SSL timeout after %d.%ds",
		  ssock->param.timeout.sec, ssock->param.timeout.msec));

	on_handshake_complete(ssock, PJ_ETIMEDOUT);
	break;
    case TIMER_CLOSE:
	pj_ssl_sock_close(ssock);
	break;
    default:
	pj_assert(!"Unknown timer");
	break;
    }
}


/* Asynchronouse handshake */
static pj_status_t do_handshake(pj_ssl_sock_t *ssock)
{
    pj_status_t status;
    int err;

    /* Perform SSL handshake */
    pj_lock_acquire(ssock->write_mutex);
    err = SSL_do_handshake(ssock->ossl_ssl);
    pj_lock_release(ssock->write_mutex);

    /* SSL_do_handshake() may put some pending data into SSL write BIO, 
     * flush it if any.
     */
    status = flush_write_bio(ssock, &ssock->handshake_op_key, 0, 0);
    if (status != PJ_SUCCESS && status != PJ_EPENDING) {
	return status;
    }

    if (err < 0) {
	int err2 = SSL_get_error(ssock->ossl_ssl, err);
	if (err2 != SSL_ERROR_NONE && err2 != SSL_ERROR_WANT_READ)
	{
	    /* Handshake fails */
	    status = STATUS_FROM_SSL_ERR2("Handshake", ssock, err, err2, 0);
	    return status;
	}
    }

    /* Check if handshake has been completed */
    if (SSL_is_init_finished(ssock->ossl_ssl)) {
	ssock->ssl_state = SSL_STATE_ESTABLISHED;
	return PJ_SUCCESS;
    }

    return PJ_EPENDING;
}

static void ssl_on_destroy(void *arg)
{
    pj_pool_t *pool = NULL;
    pj_ssl_sock_t *ssock = (pj_ssl_sock_t*)arg;

    destroy_ssl(ssock);

    pj_lock_destroy(ssock->write_mutex);

    pool = ssock->pool;
    ssock->pool = NULL;
    if (pool)
	pj_pool_release(pool);
}


/*
 *******************************************************************
 * Active socket callbacks.
 *******************************************************************
 */

static pj_bool_t asock_on_data_read (pj_activesock_t *asock,
				     void *data,
				     pj_size_t size,
				     pj_status_t status,
				     pj_size_t *remainder)
{
    pj_ssl_sock_t *ssock = (pj_ssl_sock_t*)
			   pj_activesock_get_user_data(asock);
    pj_size_t nwritten;

    /* Socket error or closed */
    if (data && size > 0) {
	/* Consume the whole data */
	nwritten = BIO_write(ssock->ossl_rbio, data, (int)size);
	if (nwritten < size) {
	    status = GET_SSL_STATUS(ssock);
	    goto on_error;
	}
    }

    /* Check if SSL handshake hasn't finished yet */
    if (ssock->ssl_state == SSL_STATE_HANDSHAKING) {
	pj_bool_t ret = PJ_TRUE;

	if (status == PJ_SUCCESS)
	    status = do_handshake(ssock);

	/* Not pending is either success or failed */
	if (status != PJ_EPENDING)
	    ret = on_handshake_complete(ssock, status);

	return ret;
    }

    /* See if there is any decrypted data for the application */
    if (ssock->read_started) {
	do {
	    read_data_t *buf = *(OFFSET_OF_READ_DATA_PTR(ssock, data));
	    void *data_ = (pj_int8_t*)buf->data + buf->len;
	    int size_ = (int)(ssock->read_size - buf->len);
	    int len = size_;

	    /* SSL_read() may write some data to BIO write when re-negotiation
	     * is on progress, so let's protect it with write mutex.
	     */
	    pj_lock_acquire(ssock->write_mutex);
	    size_ = SSL_read(ssock->ossl_ssl, data_, size_);
	    pj_lock_release(ssock->write_mutex);

	    if (size_ > 0 || status != PJ_SUCCESS) {
		if (ssock->param.cb.on_data_read) {
		    pj_bool_t ret;
		    pj_size_t remainder_ = 0;

		    if (size_ > 0)
			buf->len += size_;
    		
                    if (status != PJ_SUCCESS) {
                        ssock->ssl_state = SSL_STATE_ERROR;
                    }

		    ret = (*ssock->param.cb.on_data_read)(ssock, buf->data,
							  buf->len, status,
							  &remainder_);
		    if (!ret) {
			/* We've been destroyed */
			return PJ_FALSE;
		    }

		    /* Application may have left some data to be consumed 
		     * later.
		     */
		    buf->len = remainder_;
		}

		/* Active socket signalled connection closed/error, this has
		 * been signalled to the application along with any remaining
		 * buffer. So, let's just reset SSL socket now.
		 */
		if (status != PJ_SUCCESS) {
		    reset_ssl_sock_state(ssock);
		    return PJ_FALSE;
		}

	    } else {

		int err = SSL_get_error(ssock->ossl_ssl, size_);
		
		/* SSL might just return SSL_ERROR_WANT_READ in 
		 * re-negotiation.
		 */
		if (err != SSL_ERROR_NONE && err != SSL_ERROR_WANT_READ)
		{
		    if (err == SSL_ERROR_SYSCALL && size_ == -1 &&
			ERR_peek_error() == 0 && errno == 0)
		    {
			status = STATUS_FROM_SSL_ERR2("Read", ssock, size_,
						      err, len);
			PJ_LOG(4,("SSL", "SSL_read() = -1, with "
				  	 "SSL_ERROR_SYSCALL, no SSL error, "
				  	 "and errno = 0 - skip BIO error"));
		        /* Ignore these errors */
		    } else {
		        /* Reset SSL socket state, then return PJ_FALSE */
		        status = STATUS_FROM_SSL_ERR2("Read", ssock, size_,
		        			      err, len);
		        reset_ssl_sock_state(ssock);
		        goto on_error;
		    }
		}

		status = do_handshake(ssock);
		if (status == PJ_SUCCESS) {
		    /* Renegotiation completed */

		    /* Update certificates */
		    update_certs_info(ssock);

		    // Ticket #1573: Don't hold mutex while calling
		    //               PJLIB socket send(). 
		    //pj_lock_acquire(ssock->write_mutex);
		    status = flush_delayed_send(ssock);
		    //pj_lock_release(ssock->write_mutex);

		    /* If flushing is ongoing, treat it as success */
		    if (status == PJ_EBUSY)
			status = PJ_SUCCESS;

		    if (status != PJ_SUCCESS && status != PJ_EPENDING) {
			PJ_PERROR(1,(ssock->pool->obj_name, status, 
				     "Failed to flush delayed send"));
			goto on_error;
		    }
		} else if (status != PJ_EPENDING) {
		    PJ_PERROR(1,(ssock->pool->obj_name, status, 
			         "Renegotiation failed"));
		    goto on_error;
		}

		break;
	    }
	} while (1);
    }

    return PJ_TRUE;

on_error:
    if (ssock->ssl_state == SSL_STATE_HANDSHAKING)
	return on_handshake_complete(ssock, status);

    if (ssock->read_started && ssock->param.cb.on_data_read) {
	pj_bool_t ret;
	ret = (*ssock->param.cb.on_data_read)(ssock, NULL, 0, status,
					      remainder);
	if (!ret) {
	    /* We've been destroyed */
	    return PJ_FALSE;
	}
    }

    reset_ssl_sock_state(ssock);
    return PJ_FALSE;
}


static pj_bool_t asock_on_data_sent (pj_activesock_t *asock,
				     pj_ioqueue_op_key_t *send_key,
				     pj_ssize_t sent)
{
    pj_ssl_sock_t *ssock = (pj_ssl_sock_t*)
			   pj_activesock_get_user_data(asock);
    write_data_t *wdata = (write_data_t*)send_key->user_data;
    pj_ioqueue_op_key_t *app_key = wdata->app_key;
    pj_ssize_t sent_len;

    sent_len = (sent > 0)? wdata->plain_data_len : sent;
    
    /* Update write buffer state */
    pj_lock_acquire(ssock->write_mutex);
    free_send_data(ssock, wdata);
    pj_lock_release(ssock->write_mutex);
    wdata = NULL;

    if (ssock->ssl_state == SSL_STATE_HANDSHAKING) {
	/* Initial handshaking */
	pj_status_t status;
	
	status = do_handshake(ssock);
	/* Not pending is either success or failed */
	if (status != PJ_EPENDING)
	    return on_handshake_complete(ssock, status);

    } else if (send_key != &ssock->handshake_op_key) {
	/* Some data has been sent, notify application */
	if (ssock->param.cb.on_data_sent) {
	    pj_bool_t ret;
	    ret = (*ssock->param.cb.on_data_sent)(ssock, app_key, 
						  sent_len);
	    if (!ret) {
		/* We've been destroyed */
		return PJ_FALSE;
	    }
	}
    } else {
	/* SSL re-negotiation is on-progress, just do nothing */
    }

    /* Send buffer has been updated, let's try to send any pending data */
    if (ssock->send_buf_pending.data_len) {
	pj_status_t status;
	status = flush_write_bio(ssock, ssock->send_buf_pending.app_key,
				 ssock->send_buf_pending.plain_data_len,
				 ssock->send_buf_pending.flags);
	if (status == PJ_SUCCESS || status == PJ_EPENDING) {
	    ssock->send_buf_pending.data_len = 0;
	}
    }

    return PJ_TRUE;
}


static pj_bool_t asock_on_accept_complete (pj_activesock_t *asock,
					   pj_sock_t newsock,
					   const pj_sockaddr_t *src_addr,
					   int src_addr_len)
{
    pj_ssl_sock_t *ssock_parent = (pj_ssl_sock_t*)
				  pj_activesock_get_user_data(asock);
    pj_ssl_sock_t *ssock;
    pj_activesock_cb asock_cb;
    pj_activesock_cfg asock_cfg;
    unsigned i;
    pj_status_t status;

    /* Create new SSL socket instance */
    status = pj_ssl_sock_create(ssock_parent->pool,
				&ssock_parent->newsock_param, &ssock);
    if (status != PJ_SUCCESS)
	goto on_return;

    /* Update new SSL socket attributes */
    ssock->sock = newsock;
    ssock->parent = ssock_parent;
    ssock->is_server = PJ_TRUE;
    if (ssock_parent->cert) {
	status = pj_ssl_sock_set_certificate(ssock, ssock->pool, 
					     ssock_parent->cert);
	if (status != PJ_SUCCESS)
	    goto on_return;
    }

    /* Apply QoS, if specified */
    status = pj_sock_apply_qos2(ssock->sock, ssock->param.qos_type,
				&ssock->param.qos_params, 1, 
				ssock->pool->obj_name, NULL);
    if (status != PJ_SUCCESS && !ssock->param.qos_ignore_error)
	goto on_return;

    /* Apply socket options, if specified */
    if (ssock->param.sockopt_params.cnt) {
	status = pj_sock_setsockopt_params(ssock->sock, 
					   &ssock->param.sockopt_params);
	if (status != PJ_SUCCESS && !ssock->param.sockopt_ignore_error)
	    goto on_return;
    }

    /* Update local address */
    ssock->addr_len = src_addr_len;
    status = pj_sock_getsockname(ssock->sock, &ssock->local_addr, 
				 &ssock->addr_len);
    if (status != PJ_SUCCESS) {
	/* This fails on few envs, e.g: win IOCP, just tolerate this and
	 * use parent local address instead.
	 */
	pj_sockaddr_cp(&ssock->local_addr, &ssock_parent->local_addr);
    }

    /* Set remote address */
    pj_sockaddr_cp(&ssock->rem_addr, src_addr);

    /* Create SSL context */
    status = create_ssl(ssock);
    if (status != PJ_SUCCESS)
	goto on_return;

    /* Prepare read buffer */
    ssock->asock_rbuf = (void**)pj_pool_calloc(ssock->pool, 
					       ssock->param.async_cnt,
					       sizeof(void*));
    for (i = 0; i<ssock->param.async_cnt; ++i) {
	ssock->asock_rbuf[i] = (void*) pj_pool_alloc(
					    ssock->pool, 
					    ssock->param.read_buffer_size + 
					    sizeof(read_data_t*));
    }

    /* Create active socket */
    pj_activesock_cfg_default(&asock_cfg);
    asock_cfg.async_cnt = ssock->param.async_cnt;
    asock_cfg.concurrency = ssock->param.concurrency;
    asock_cfg.whole_data = PJ_TRUE;
    
    /* If listener socket has group lock, automatically create group lock
     * for the new socket.
     */
    if (ssock_parent->param.grp_lock) {
	pj_grp_lock_t *glock;

	status = pj_grp_lock_create(ssock->pool, NULL, &glock);
	if (status != PJ_SUCCESS)
	    goto on_return;

	pj_grp_lock_add_ref(glock);
	asock_cfg.grp_lock = ssock->param.grp_lock = glock;
	pj_grp_lock_add_handler(ssock->param.grp_lock, ssock->pool, ssock,
				ssl_on_destroy);
    }

    pj_bzero(&asock_cb, sizeof(asock_cb));
    asock_cb.on_data_read = asock_on_data_read;
    asock_cb.on_data_sent = asock_on_data_sent;

    status = pj_activesock_create(ssock->pool,
				  ssock->sock, 
				  ssock->param.sock_type,
				  &asock_cfg,
				  ssock->param.ioqueue, 
				  &asock_cb,
				  ssock,
				  &ssock->asock);

    if (status != PJ_SUCCESS)
	goto on_return;

    /* Start read */
    status = pj_activesock_start_read2(ssock->asock, ssock->pool, 
				       (unsigned)ssock->param.read_buffer_size,
				       ssock->asock_rbuf,
				       PJ_IOQUEUE_ALWAYS_ASYNC);
    if (status != PJ_SUCCESS)
	goto on_return;

    /* Prepare write/send state */
    pj_assert(ssock->send_buf.max_len == 0);
    ssock->send_buf.buf = (char*)
			  pj_pool_alloc(ssock->pool, 
					ssock->param.send_buffer_size);
    ssock->send_buf.max_len = ssock->param.send_buffer_size;
    ssock->send_buf.start = ssock->send_buf.buf;
    ssock->send_buf.len = 0;

    /* Start handshake timer */
    if (ssock->param.timer_heap && (ssock->param.timeout.sec != 0 ||
	ssock->param.timeout.msec != 0))
    {
	pj_assert(ssock->timer.id == TIMER_NONE);
	ssock->timer.id = TIMER_HANDSHAKE_TIMEOUT;
	status = pj_timer_heap_schedule(ssock->param.timer_heap, 
				        &ssock->timer,
					&ssock->param.timeout);
	if (status != PJ_SUCCESS) {
	    ssock->timer.id = TIMER_NONE;
	    status = PJ_SUCCESS;
	}
    }

    /* Start SSL handshake */
    ssock->ssl_state = SSL_STATE_HANDSHAKING;
    SSL_set_accept_state(ssock->ossl_ssl);
    status = do_handshake(ssock);

on_return:
    if (ssock && status != PJ_EPENDING) {
	on_handshake_complete(ssock, status);
    }

    /* Must return PJ_TRUE whatever happened, as active socket must 
     * continue listening.
     */
    return PJ_TRUE;
}


static pj_bool_t asock_on_accept_complete2(pj_activesock_t *asock,
					   pj_sock_t newsock,
					   const pj_sockaddr_t *src_addr,
					   int src_addr_len,
					   pj_status_t status)
{
    pj_bool_t ret = PJ_TRUE;
    if (status != PJ_SUCCESS) {
	pj_ssl_sock_t *ssock = (pj_ssl_sock_t*)
			pj_activesock_get_user_data(asock);

	if (ssock->param.cb.on_accept_complete2) {
	    (*ssock->param.cb.on_accept_complete2) (ssock->parent, ssock, 
						    src_addr, src_addr_len, 
						    status);
	}
    } else {
	ret = asock_on_accept_complete(asock, newsock, src_addr, src_addr_len);
    }
    return ret;
}


static pj_bool_t asock_on_connect_complete (pj_activesock_t *asock,
					    pj_status_t status)
{
    pj_ssl_sock_t *ssock = (pj_ssl_sock_t*)
			   pj_activesock_get_user_data(asock);
    unsigned i;

    if (status != PJ_SUCCESS)
	goto on_return;

    /* Update local address */
    ssock->addr_len = sizeof(pj_sockaddr);
    status = pj_sock_getsockname(ssock->sock, &ssock->local_addr, 
				 &ssock->addr_len);
    if (status != PJ_SUCCESS)
	goto on_return;

    /* Create SSL context */
    status = create_ssl(ssock);
    if (status != PJ_SUCCESS)
	goto on_return;

    /* Prepare read buffer */
    ssock->asock_rbuf = (void**)pj_pool_calloc(ssock->pool, 
					       ssock->param.async_cnt,
					       sizeof(void*));
    for (i = 0; i<ssock->param.async_cnt; ++i) {
	ssock->asock_rbuf[i] = (void*) pj_pool_alloc(
					    ssock->pool, 
					    ssock->param.read_buffer_size + 
					    sizeof(read_data_t*));
    }

    /* Start read */
    status = pj_activesock_start_read2(ssock->asock, ssock->pool, 
				       (unsigned)ssock->param.read_buffer_size,
				       ssock->asock_rbuf,
				       PJ_IOQUEUE_ALWAYS_ASYNC);
    if (status != PJ_SUCCESS)
	goto on_return;

    /* Prepare write/send state */
    pj_assert(ssock->send_buf.max_len == 0);
    ssock->send_buf.buf = (char*)
			     pj_pool_alloc(ssock->pool, 
					   ssock->param.send_buffer_size);
    ssock->send_buf.max_len = ssock->param.send_buffer_size;
    ssock->send_buf.start = ssock->send_buf.buf;
    ssock->send_buf.len = 0;

#ifdef SSL_set_tlsext_host_name
    /* Set server name to connect */
    if (ssock->param.server_name.slen) {
	/* Server name is null terminated already */
	if (!SSL_set_tlsext_host_name(ssock->ossl_ssl, 
				      ssock->param.server_name.ptr))
	{
	    char err_str[PJ_ERR_MSG_SIZE];

	    ERR_error_string_n(ERR_get_error(), err_str, sizeof(err_str));
	    PJ_LOG(3,(ssock->pool->obj_name, "SSL_set_tlsext_host_name() "
		"failed: %s", err_str));
	}
    }
#endif

    /* Start SSL handshake */
    ssock->ssl_state = SSL_STATE_HANDSHAKING;
    SSL_set_connect_state(ssock->ossl_ssl);

    status = do_handshake(ssock);
    if (status != PJ_EPENDING)
	goto on_return;

    return PJ_TRUE;

on_return:
    return on_handshake_complete(ssock, status);
}



/*
 *******************************************************************
 * API
 *******************************************************************
 */

/* Load credentials from files. */
PJ_DEF(pj_status_t) pj_ssl_cert_load_from_files (pj_pool_t *pool,
						 const pj_str_t *CA_file,
						 const pj_str_t *cert_file,
						 const pj_str_t *privkey_file,
						 const pj_str_t *privkey_pass,
						 pj_ssl_cert_t **p_cert)
{
    return pj_ssl_cert_load_from_files2(pool, CA_file, NULL, cert_file,
					privkey_file, privkey_pass, p_cert);
}

PJ_DEF(pj_status_t) pj_ssl_cert_load_from_files2(pj_pool_t *pool,
						 const pj_str_t *CA_file,
						 const pj_str_t *CA_path,
						 const pj_str_t *cert_file,
						 const pj_str_t *privkey_file,
						 const pj_str_t *privkey_pass,
						 pj_ssl_cert_t **p_cert)
{
    pj_ssl_cert_t *cert;

    PJ_ASSERT_RETURN(pool && (CA_file || CA_path) && cert_file &&
		     privkey_file,
		     PJ_EINVAL);

    cert = PJ_POOL_ZALLOC_T(pool, pj_ssl_cert_t);
    if (CA_file) {
    	pj_strdup_with_null(pool, &cert->CA_file, CA_file);
    }
    if (CA_path) {
    	pj_strdup_with_null(pool, &cert->CA_path, CA_path);
    }
    pj_strdup_with_null(pool, &cert->cert_file, cert_file);
    pj_strdup_with_null(pool, &cert->privkey_file, privkey_file);
    pj_strdup_with_null(pool, &cert->privkey_pass, privkey_pass);

    *p_cert = cert;

    return PJ_SUCCESS;
}

PJ_DEF(pj_status_t) pj_ssl_cert_load_from_buffer(pj_pool_t *pool,
					const pj_ssl_cert_buffer *CA_buf,
					const pj_ssl_cert_buffer *cert_buf,
					const pj_ssl_cert_buffer *privkey_buf,
					const pj_str_t *privkey_pass,
					pj_ssl_cert_t **p_cert)
{
    pj_ssl_cert_t *cert;

    PJ_ASSERT_RETURN(pool && CA_buf && cert_buf && privkey_buf, PJ_EINVAL);

    cert = PJ_POOL_ZALLOC_T(pool, pj_ssl_cert_t);
    pj_strdup(pool, &cert->CA_buf, CA_buf);
    pj_strdup(pool, &cert->cert_buf, cert_buf);
    pj_strdup(pool, &cert->privkey_buf, privkey_buf);
    pj_strdup_with_null(pool, &cert->privkey_pass, privkey_pass);

    *p_cert = cert;

    return PJ_SUCCESS;
}

/* Set SSL socket credentials. */
PJ_DEF(pj_status_t) pj_ssl_sock_set_certificate(
					    pj_ssl_sock_t *ssock,
					    pj_pool_t *pool,
					    const pj_ssl_cert_t *cert)
{
    pj_ssl_cert_t *cert_;

    PJ_ASSERT_RETURN(ssock && pool && cert, PJ_EINVAL);

    cert_ = PJ_POOL_ZALLOC_T(pool, pj_ssl_cert_t);
    pj_memcpy(cert_, cert, sizeof(pj_ssl_cert_t));
    pj_strdup_with_null(pool, &cert_->CA_file, &cert->CA_file);
    pj_strdup_with_null(pool, &cert_->CA_path, &cert->CA_path);
    pj_strdup_with_null(pool, &cert_->cert_file, &cert->cert_file);
    pj_strdup_with_null(pool, &cert_->privkey_file, &cert->privkey_file);
    pj_strdup_with_null(pool, &cert_->privkey_pass, &cert->privkey_pass);

    pj_strdup(pool, &cert_->CA_buf, &cert->CA_buf);
    pj_strdup(pool, &cert_->cert_buf, &cert->cert_buf);
    pj_strdup(pool, &cert_->privkey_buf, &cert->privkey_buf);

    ssock->cert = cert_;

    return PJ_SUCCESS;
}


/* Get available ciphers. */
PJ_DEF(pj_status_t) pj_ssl_cipher_get_availables(pj_ssl_cipher ciphers[],
					         unsigned *cipher_num)
{
    unsigned i;

    PJ_ASSERT_RETURN(ciphers && cipher_num, PJ_EINVAL);

    if (openssl_cipher_num == 0) {
	init_openssl();
	shutdown_openssl();
    }

    if (openssl_cipher_num == 0) {
	*cipher_num = 0;
	return PJ_ENOTFOUND;
    }

    *cipher_num = PJ_MIN(*cipher_num, openssl_cipher_num);

    for (i = 0; i < *cipher_num; ++i)
	ciphers[i] = openssl_ciphers[i].id;

    return PJ_SUCCESS;
}


/* Get cipher name string */
PJ_DEF(const char*) pj_ssl_cipher_name(pj_ssl_cipher cipher)
{
    unsigned i;

    if (openssl_cipher_num == 0) {
	init_openssl();
	shutdown_openssl();
    }

    for (i = 0; i < openssl_cipher_num; ++i) {
	if (cipher == openssl_ciphers[i].id)
	    return openssl_ciphers[i].name;
    }

    return NULL;
}

/* Get cipher identifier */
PJ_DEF(pj_ssl_cipher) pj_ssl_cipher_id(const char *cipher_name)
{
    unsigned i;

    if (openssl_cipher_num == 0) {
        init_openssl();
        shutdown_openssl();
    }

    for (i = 0; i < openssl_cipher_num; ++i) {
        if (!pj_ansi_stricmp(openssl_ciphers[i].name, cipher_name))
            return openssl_ciphers[i].id;
    }

    return PJ_TLS_UNKNOWN_CIPHER;
}

/* Check if the specified cipher is supported by SSL/TLS backend. */
PJ_DEF(pj_bool_t) pj_ssl_cipher_is_supported(pj_ssl_cipher cipher)
{
    unsigned i;

    if (openssl_cipher_num == 0) {
	init_openssl();
	shutdown_openssl();
    }

    for (i = 0; i < openssl_cipher_num; ++i) {
	if (cipher == openssl_ciphers[i].id)
	    return PJ_TRUE;
    }

    return PJ_FALSE;
}

/* Get available curves. */
PJ_DEF(pj_status_t) pj_ssl_curve_get_availables(pj_ssl_curve curves[],
						unsigned *curve_num)
{
    unsigned i;

    PJ_ASSERT_RETURN(curves && curve_num, PJ_EINVAL);

    if (openssl_curves_num == 0) {
	init_openssl();
	shutdown_openssl();
    }

    if (openssl_curves_num == 0) {
	*curve_num = 0;
	return PJ_ENOTFOUND;
    }

    *curve_num = PJ_MIN(*curve_num, openssl_curves_num);

    for (i = 0; i < *curve_num; ++i)
	curves[i] = openssl_curves[i].id;

    return PJ_SUCCESS;
}

/* Get curve name string. */
PJ_DEF(const char*) pj_ssl_curve_name(pj_ssl_curve curve)
{
    unsigned i;

    if (openssl_curves_num == 0) {
	init_openssl();
	shutdown_openssl();
    }

    for (i = 0; i < openssl_curves_num; ++i) {
	if (curve == openssl_curves[i].id)
	    return openssl_curves[i].name;
    }

    return NULL;
}

/* Get curve ID from curve name string. */
PJ_DEF(pj_ssl_curve) pj_ssl_curve_id(const char *curve_name)
{
    unsigned i;

    if (openssl_curves_num == 0) {
        init_openssl();
        shutdown_openssl();
    }

    for (i = 0; i < openssl_curves_num; ++i) {
        if (openssl_curves[i].name &&
        	!pj_ansi_stricmp(openssl_curves[i].name, curve_name))
        {
            return openssl_curves[i].id;
        }
    }

    return PJ_TLS_UNKNOWN_CURVE;
}

/* Check if the specified curve is supported by SSL/TLS backend. */
PJ_DEF(pj_bool_t) pj_ssl_curve_is_supported(pj_ssl_curve curve)
{
    unsigned i;

    if (openssl_curves_num == 0) {
	init_openssl();
	shutdown_openssl();
    }

    for (i = 0; i < openssl_curves_num; ++i) {
	if (curve == openssl_curves[i].id)
	    return PJ_TRUE;
    }

    return PJ_FALSE;
}

/*
 * Create SSL socket instance. 
 */
PJ_DEF(pj_status_t) pj_ssl_sock_create (pj_pool_t *pool,
					const pj_ssl_sock_param *param,
					pj_ssl_sock_t **p_ssock)
{
    pj_ssl_sock_t *ssock;
    pj_status_t status;

    PJ_ASSERT_RETURN(pool && param && p_ssock, PJ_EINVAL);
    PJ_ASSERT_RETURN(param->sock_type == pj_SOCK_STREAM(), PJ_ENOTSUP);

    pool = pj_pool_create(pool->factory, "ssl%p", 512, 512, NULL);

    /* Create secure socket */
    ssock = PJ_POOL_ZALLOC_T(pool, pj_ssl_sock_t);
    ssock->pool = pool;
    ssock->sock = PJ_INVALID_SOCKET;
    ssock->ssl_state = SSL_STATE_NULL;
    pj_list_init(&ssock->write_pending);
    pj_list_init(&ssock->write_pending_empty);
    pj_list_init(&ssock->send_pending);
    pj_timer_entry_init(&ssock->timer, 0, ssock, &on_timer);
    pj_ioqueue_op_key_init(&ssock->handshake_op_key,
			   sizeof(pj_ioqueue_op_key_t));

    /* Create secure socket mutex */
    status = pj_lock_create_recursive_mutex(pool, pool->obj_name,
					    &ssock->write_mutex);
    if (status != PJ_SUCCESS) {
	pj_pool_release(pool);
	return status;
    }

    /* Init secure socket param */
    pj_ssl_sock_param_copy(pool, &ssock->param, param);

    if (ssock->param.grp_lock) {
	pj_grp_lock_add_ref(ssock->param.grp_lock);
	pj_grp_lock_add_handler(ssock->param.grp_lock, pool, ssock,
				ssl_on_destroy);
    }

    ssock->param.read_buffer_size = ((ssock->param.read_buffer_size+7)>>3)<<3;
    if (!ssock->param.timer_heap) {
	PJ_LOG(3,(ssock->pool->obj_name, "Warning: timer heap is not "
		  "available. It is recommended to supply one to avoid "
	          "a race condition if more than one worker threads "
	          "are used."));
    }

    /* Finally */
    *p_ssock = ssock;

    return PJ_SUCCESS;
}


/*
 * Close the secure socket. This will unregister the socket from the
 * ioqueue and ultimately close the socket.
 */
PJ_DEF(pj_status_t) pj_ssl_sock_close(pj_ssl_sock_t *ssock)
{
    PJ_ASSERT_RETURN(ssock, PJ_EINVAL);

    if (!ssock->pool)
	return PJ_SUCCESS;

    if (ssock->timer.id != TIMER_NONE) {
	pj_timer_heap_cancel(ssock->param.timer_heap, &ssock->timer);
	ssock->timer.id = TIMER_NONE;
    }

    reset_ssl_sock_state(ssock);
    if (ssock->param.grp_lock) {
	pj_grp_lock_dec_ref(ssock->param.grp_lock);
    } else {
	ssl_on_destroy(ssock);
    }

    return PJ_SUCCESS;
}


/*
 * Associate arbitrary data with the secure socket.
 */
PJ_DEF(pj_status_t) pj_ssl_sock_set_user_data(pj_ssl_sock_t *ssock,
					      void *user_data)
{
    PJ_ASSERT_RETURN(ssock, PJ_EINVAL);

    ssock->param.user_data = user_data;
    return PJ_SUCCESS;
}


/*
 * Retrieve the user data previously associated with this secure
 * socket.
 */
PJ_DEF(void*) pj_ssl_sock_get_user_data(pj_ssl_sock_t *ssock)
{
    PJ_ASSERT_RETURN(ssock, NULL);

    return ssock->param.user_data;
}


/*
 * Retrieve the local address and port used by specified SSL socket.
 */
PJ_DEF(pj_status_t) pj_ssl_sock_get_info (pj_ssl_sock_t *ssock,
					  pj_ssl_sock_info *info)
{
    pj_bzero(info, sizeof(*info));

    /* Established flag */
    info->established = (ssock->ssl_state == SSL_STATE_ESTABLISHED);

    /* Protocol */
    info->proto = ssock->param.proto;

    /* Local address */
    pj_sockaddr_cp(&info->local_addr, &ssock->local_addr);
    
    if (info->established) {
	const SSL_CIPHER *cipher;

	/* Current cipher */
	cipher = SSL_get_current_cipher(ssock->ossl_ssl);
	if (cipher) {
	    info->cipher = (SSL_CIPHER_get_id(cipher) & 0x00FFFFFF);
	} else {
	    info->cipher = PJ_TLS_UNKNOWN_CIPHER;
	}

	/* Remote address */
	pj_sockaddr_cp(&info->remote_addr, &ssock->rem_addr);

	/* Certificates info */
	info->local_cert_info = &ssock->local_cert_info;
	info->remote_cert_info = &ssock->remote_cert_info;

	/* Verification status */
	info->verify_status = ssock->verify_status;
    }

    /* Last known OpenSSL error code */
    info->last_native_err = ssock->last_err;

    /* Group lock */
    info->grp_lock = ssock->param.grp_lock;

    return PJ_SUCCESS;
}


/*
 * Starts read operation on this secure socket.
 */
PJ_DEF(pj_status_t) pj_ssl_sock_start_read (pj_ssl_sock_t *ssock,
					    pj_pool_t *pool,
					    unsigned buff_size,
					    pj_uint32_t flags)
{
    void **readbuf;
    unsigned i;

    PJ_ASSERT_RETURN(ssock && pool && buff_size, PJ_EINVAL);

    if (ssock->ssl_state != SSL_STATE_ESTABLISHED) 
	return PJ_EINVALIDOP;

    readbuf = (void**) pj_pool_calloc(pool, ssock->param.async_cnt, 
				      sizeof(void*));

    for (i=0; i<ssock->param.async_cnt; ++i) {
	readbuf[i] = pj_pool_alloc(pool, buff_size);
    }

    return pj_ssl_sock_start_read2(ssock, pool, buff_size, 
				   readbuf, flags);
}


/*
 * Same as #pj_ssl_sock_start_read(), except that the application
 * supplies the buffers for the read operation so that the acive socket
 * does not have to allocate the buffers.
 */
PJ_DEF(pj_status_t) pj_ssl_sock_start_read2 (pj_ssl_sock_t *ssock,
					     pj_pool_t *pool,
					     unsigned buff_size,
					     void *readbuf[],
					     pj_uint32_t flags)
{
    unsigned i;

    PJ_ASSERT_RETURN(ssock && pool && buff_size && readbuf, PJ_EINVAL);

    if (ssock->ssl_state != SSL_STATE_ESTABLISHED) 
	return PJ_EINVALIDOP;

    /* Create SSL socket read buffer */
    ssock->ssock_rbuf = (read_data_t*)pj_pool_calloc(pool, 
					       ssock->param.async_cnt,
					       sizeof(read_data_t));

    /* Store SSL socket read buffer pointer in the activesock read buffer */
    for (i=0; i<ssock->param.async_cnt; ++i) {
	read_data_t **p_ssock_rbuf = 
			OFFSET_OF_READ_DATA_PTR(ssock, ssock->asock_rbuf[i]);

	ssock->ssock_rbuf[i].data = readbuf[i];
	ssock->ssock_rbuf[i].len = 0;

	*p_ssock_rbuf = &ssock->ssock_rbuf[i];
    }

    ssock->read_size = buff_size;
    ssock->read_started = PJ_TRUE;
    ssock->read_flags = flags;

    return PJ_SUCCESS;
}


/*
 * Same as pj_ssl_sock_start_read(), except that this function is used
 * only for datagram sockets, and it will trigger \a on_data_recvfrom()
 * callback instead.
 */
PJ_DEF(pj_status_t) pj_ssl_sock_start_recvfrom (pj_ssl_sock_t *ssock,
						pj_pool_t *pool,
						unsigned buff_size,
						pj_uint32_t flags)
{
    PJ_UNUSED_ARG(ssock);
    PJ_UNUSED_ARG(pool);
    PJ_UNUSED_ARG(buff_size);
    PJ_UNUSED_ARG(flags);

    return PJ_ENOTSUP;
}


/*
 * Same as #pj_ssl_sock_start_recvfrom() except that the recvfrom() 
 * operation takes the buffer from the argument rather than creating
 * new ones.
 */
PJ_DEF(pj_status_t) pj_ssl_sock_start_recvfrom2 (pj_ssl_sock_t *ssock,
						 pj_pool_t *pool,
						 unsigned buff_size,
						 void *readbuf[],
						 pj_uint32_t flags)
{
    PJ_UNUSED_ARG(ssock);
    PJ_UNUSED_ARG(pool);
    PJ_UNUSED_ARG(buff_size);
    PJ_UNUSED_ARG(readbuf);
    PJ_UNUSED_ARG(flags);

    return PJ_ENOTSUP;
}

/* Write plain data to SSL and flush write BIO. */
static pj_status_t ssl_write(pj_ssl_sock_t *ssock, 
			     pj_ioqueue_op_key_t *send_key,
			     const void *data,
			     pj_ssize_t size,
			     unsigned flags)
{
    pj_status_t status;
    int nwritten;

    /* Write the plain data to SSL, after SSL encrypts it, write BIO will
     * contain the secured data to be sent via socket. Note that re-
     * negotitation may be on progress, so sending data should be delayed
     * until re-negotiation is completed.
     */
    pj_lock_acquire(ssock->write_mutex);
    /* Don't write to SSL if send buffer is full and some data is in
     * write BIO already, just return PJ_ENOMEM.
     */
    if (ssock->send_buf_pending.data_len) {
	pj_lock_release(ssock->write_mutex);
	return PJ_ENOMEM;
    }
    nwritten = SSL_write(ssock->ossl_ssl, data, (int)size);
    pj_lock_release(ssock->write_mutex);
    
    if (nwritten == size) {
	/* All data written, flush write BIO to network socket */
	status = flush_write_bio(ssock, send_key, size, flags);
    } else if (nwritten <= 0) {
	/* SSL failed to process the data, it may just that re-negotiation
	 * is on progress.
	 */
	int err;
	err = SSL_get_error(ssock->ossl_ssl, nwritten);
	if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_NONE) {
	    /* Re-negotiation is on progress, flush re-negotiation data */
	    status = flush_write_bio(ssock, &ssock->handshake_op_key, 0, 0);
	    if (status == PJ_SUCCESS || status == PJ_EPENDING)
		/* Just return PJ_EBUSY when re-negotiation is on progress */
		status = PJ_EBUSY;
	} else {
	    /* Some problem occured */
	    status = STATUS_FROM_SSL_ERR2("Write", ssock, nwritten, err, size);
	}
    } else {
	/* nwritten < *size, shouldn't happen, unless write BIO cannot hold 
	 * the whole secured data, perhaps because of insufficient memory.
	 */
	status = PJ_ENOMEM;
    }

    return status;
}

/* Flush delayed data sending in the write pending list. */
static pj_status_t flush_delayed_send(pj_ssl_sock_t *ssock)
{
    /* Check for another ongoing flush */
    if (ssock->flushing_write_pend)
	return PJ_EBUSY;

    pj_lock_acquire(ssock->write_mutex);

    /* Again, check for another ongoing flush */
    if (ssock->flushing_write_pend) {
	pj_lock_release(ssock->write_mutex);
	return PJ_EBUSY;
    }

    /* Set ongoing flush flag */
    ssock->flushing_write_pend = PJ_TRUE;

    while (!pj_list_empty(&ssock->write_pending)) {
        write_data_t *wp;
	pj_status_t status;

	wp = ssock->write_pending.next;

	/* Ticket #1573: Don't hold mutex while calling socket send. */
	pj_lock_release(ssock->write_mutex);

	status = ssl_write(ssock, &wp->key, wp->data.ptr, 
			   wp->plain_data_len, wp->flags);
	if (status != PJ_SUCCESS) {
	    /* Reset ongoing flush flag first. */
	    ssock->flushing_write_pend = PJ_FALSE;
	    return status;
	}

	pj_lock_acquire(ssock->write_mutex);
	pj_list_erase(wp);
	pj_list_push_back(&ssock->write_pending_empty, wp);
    }

    /* Reset ongoing flush flag */
    ssock->flushing_write_pend = PJ_FALSE;

    pj_lock_release(ssock->write_mutex);

    return PJ_SUCCESS;
}

/* Sending is delayed, push back the sending data into pending list. */
static pj_status_t delay_send (pj_ssl_sock_t *ssock,
			       pj_ioqueue_op_key_t *send_key,
			       const void *data,
			       pj_ssize_t size,
			       unsigned flags)
{
    write_data_t *wp;

    pj_lock_acquire(ssock->write_mutex);

    /* Init write pending instance */
    if (!pj_list_empty(&ssock->write_pending_empty)) {
	wp = ssock->write_pending_empty.next;
	pj_list_erase(wp);
    } else {
	wp = PJ_POOL_ZALLOC_T(ssock->pool, write_data_t);
    }

    wp->app_key = send_key;
    wp->plain_data_len = size;
    wp->data.ptr = data;
    wp->flags = flags;

    pj_list_push_back(&ssock->write_pending, wp);
    
    pj_lock_release(ssock->write_mutex);

    /* Must return PJ_EPENDING */
    return PJ_EPENDING;
}

/**
 * Send data using the socket.
 */
PJ_DEF(pj_status_t) pj_ssl_sock_send (pj_ssl_sock_t *ssock,
				      pj_ioqueue_op_key_t *send_key,
				      const void *data,
				      pj_ssize_t *size,
				      unsigned flags)
{
    pj_status_t status;

    PJ_ASSERT_RETURN(ssock && data && size && (*size>0), PJ_EINVAL);

    if (ssock->ssl_state != SSL_STATE_ESTABLISHED) 
	return PJ_EINVALIDOP;

    // Ticket #1573: Don't hold mutex while calling PJLIB socket send().
    //pj_lock_acquire(ssock->write_mutex);

    /* Flush delayed send first. Sending data might be delayed when 
     * re-negotiation is on-progress.
     */
    status = flush_delayed_send(ssock);
    if (status == PJ_EBUSY) {
	/* Re-negotiation or flushing is on progress, delay sending */
	status = delay_send(ssock, send_key, data, *size, flags);
	goto on_return;
    } else if (status != PJ_SUCCESS) {
	goto on_return;
    }

    /* Write data to SSL */
    status = ssl_write(ssock, send_key, data, *size, flags);
    if (status == PJ_EBUSY) {
	/* Re-negotiation is on progress, delay sending */
	status = delay_send(ssock, send_key, data, *size, flags);
    }

on_return:
    //pj_lock_release(ssock->write_mutex);
    return status;
}


/**
 * Send datagram using the socket.
 */
PJ_DEF(pj_status_t) pj_ssl_sock_sendto (pj_ssl_sock_t *ssock,
					pj_ioqueue_op_key_t *send_key,
					const void *data,
					pj_ssize_t *size,
					unsigned flags,
					const pj_sockaddr_t *addr,
					int addr_len)
{
    PJ_UNUSED_ARG(ssock);
    PJ_UNUSED_ARG(send_key);
    PJ_UNUSED_ARG(data);
    PJ_UNUSED_ARG(size);
    PJ_UNUSED_ARG(flags);
    PJ_UNUSED_ARG(addr);
    PJ_UNUSED_ARG(addr_len);

    return PJ_ENOTSUP;
}


/**
 * Starts asynchronous socket accept() operations on this secure socket. 
 */
PJ_DEF(pj_status_t) pj_ssl_sock_start_accept (pj_ssl_sock_t *ssock,
					      pj_pool_t *pool,
					      const pj_sockaddr_t *localaddr,
					      int addr_len)
{
    return pj_ssl_sock_start_accept2(ssock, pool, localaddr, addr_len,
    				     &ssock->param);
}


/**
 * Same as #pj_ssl_sock_start_accept(), but application provides parameter
 * for new accepted secure sockets.
 */
PJ_DEF(pj_status_t)
pj_ssl_sock_start_accept2(pj_ssl_sock_t *ssock,
			  pj_pool_t *pool,
			  const pj_sockaddr_t *localaddr,
			  int addr_len,
			  const pj_ssl_sock_param *newsock_param)
{
    pj_activesock_cb asock_cb;
    pj_activesock_cfg asock_cfg;
    pj_status_t status;

    PJ_ASSERT_RETURN(ssock && pool && localaddr && addr_len, PJ_EINVAL);

    /* Verify new socket parameters */
    if (newsock_param->grp_lock != ssock->param.grp_lock ||
        newsock_param->sock_af != ssock->param.sock_af ||
        newsock_param->sock_type != ssock->param.sock_type)
    {
        return PJ_EINVAL;
    }

    /* Create socket */
    status = pj_sock_socket(ssock->param.sock_af, ssock->param.sock_type, 0, 
			    &ssock->sock);
    if (status != PJ_SUCCESS)
	goto on_error;

    /* Apply SO_REUSEADDR */
    if (ssock->param.reuse_addr) {
	int enabled = 1;
	status = pj_sock_setsockopt(ssock->sock, pj_SOL_SOCKET(),
				    pj_SO_REUSEADDR(),
				    &enabled, sizeof(enabled));
	if (status != PJ_SUCCESS) {
	    PJ_PERROR(4,(ssock->pool->obj_name, status,
		         "Warning: error applying SO_REUSEADDR"));
	}
    }

    /* Apply QoS, if specified */
    status = pj_sock_apply_qos2(ssock->sock, ssock->param.qos_type,
				&ssock->param.qos_params, 2, 
				ssock->pool->obj_name, NULL);

    if (status != PJ_SUCCESS && !ssock->param.qos_ignore_error)
	goto on_error;

    /* Apply socket options, if specified */
    if (ssock->param.sockopt_params.cnt) {
	status = pj_sock_setsockopt_params(ssock->sock, 
					   &ssock->param.sockopt_params);

	if (status != PJ_SUCCESS && !ssock->param.sockopt_ignore_error)
	    goto on_error;
    }

    /* Bind socket */
    status = pj_sock_bind(ssock->sock, localaddr, addr_len);
    if (status != PJ_SUCCESS)
	goto on_error;

    /* Start listening to the address */
    status = pj_sock_listen(ssock->sock, PJ_SOMAXCONN);
    if (status != PJ_SUCCESS)
	goto on_error;

    /* Create active socket */
    pj_activesock_cfg_default(&asock_cfg);
    asock_cfg.async_cnt = ssock->param.async_cnt;
    asock_cfg.concurrency = ssock->param.concurrency;
    asock_cfg.whole_data = PJ_TRUE;
    asock_cfg.grp_lock = ssock->param.grp_lock;

    pj_bzero(&asock_cb, sizeof(asock_cb));
    //asock_cb.on_accept_complete = asock_on_accept_complete;
    asock_cb.on_accept_complete2 = asock_on_accept_complete2;

    status = pj_activesock_create(pool,
				  ssock->sock, 
				  ssock->param.sock_type,
				  &asock_cfg,
				  ssock->param.ioqueue, 
				  &asock_cb,
				  ssock,
				  &ssock->asock);

    if (status != PJ_SUCCESS)
	goto on_error;

    /* Start accepting */
    pj_ssl_sock_param_copy(pool, &ssock->newsock_param, newsock_param);
    ssock->newsock_param.grp_lock = NULL;
    status = pj_activesock_start_accept(ssock->asock, pool);
    if (status != PJ_SUCCESS)
	goto on_error;

    /* Update local address */
    ssock->addr_len = addr_len;
    status = pj_sock_getsockname(ssock->sock, &ssock->local_addr, 
				 &ssock->addr_len);
    if (status != PJ_SUCCESS)
	pj_sockaddr_cp(&ssock->local_addr, localaddr);

    ssock->is_server = PJ_TRUE;

    return PJ_SUCCESS;

on_error:
    reset_ssl_sock_state(ssock);
    return status;
}


/**
 * Starts asynchronous socket connect() operation.
 */
PJ_DEF(pj_status_t) pj_ssl_sock_start_connect( pj_ssl_sock_t *ssock,
					       pj_pool_t *pool,
					       const pj_sockaddr_t *localaddr,
					       const pj_sockaddr_t *remaddr,
					       int addr_len)
{
    pj_activesock_cb asock_cb;
    pj_activesock_cfg asock_cfg;
    pj_status_t status;

    PJ_ASSERT_RETURN(ssock && pool && localaddr && remaddr && addr_len,
		     PJ_EINVAL);

    /* Create socket */
    status = pj_sock_socket(ssock->param.sock_af, ssock->param.sock_type, 0, 
			    &ssock->sock);
    if (status != PJ_SUCCESS)
	goto on_error;

    /* Apply QoS, if specified */
    status = pj_sock_apply_qos2(ssock->sock, ssock->param.qos_type,
				&ssock->param.qos_params, 2, 
				ssock->pool->obj_name, NULL);
    if (status != PJ_SUCCESS && !ssock->param.qos_ignore_error)
	goto on_error;

    /* Apply socket options, if specified */
    if (ssock->param.sockopt_params.cnt) {
	status = pj_sock_setsockopt_params(ssock->sock, 
					   &ssock->param.sockopt_params);

	if (status != PJ_SUCCESS && !ssock->param.sockopt_ignore_error)
	    goto on_error;
    }

    /* Bind socket */
    status = pj_sock_bind(ssock->sock, localaddr, addr_len);
    if (status != PJ_SUCCESS)
	goto on_error;

    /* Create active socket */
    pj_activesock_cfg_default(&asock_cfg);
    asock_cfg.async_cnt = ssock->param.async_cnt;
    asock_cfg.concurrency = ssock->param.concurrency;
    asock_cfg.whole_data = PJ_TRUE;
    asock_cfg.grp_lock = ssock->param.grp_lock;

    pj_bzero(&asock_cb, sizeof(asock_cb));
    asock_cb.on_connect_complete = asock_on_connect_complete;
    asock_cb.on_data_read = asock_on_data_read;
    asock_cb.on_data_sent = asock_on_data_sent;

    status = pj_activesock_create(pool,
				  ssock->sock, 
				  ssock->param.sock_type,
				  &asock_cfg,
				  ssock->param.ioqueue, 
				  &asock_cb,
				  ssock,
				  &ssock->asock);

    if (status != PJ_SUCCESS)
	goto on_error;

    /* Save remote address */
    pj_sockaddr_cp(&ssock->rem_addr, remaddr);

    /* Start timer */
    if (ssock->param.timer_heap && (ssock->param.timeout.sec != 0 ||
	ssock->param.timeout.msec != 0))
    {
	pj_assert(ssock->timer.id == TIMER_NONE);
	ssock->timer.id = TIMER_HANDSHAKE_TIMEOUT;
	status = pj_timer_heap_schedule(ssock->param.timer_heap,
					&ssock->timer,
				        &ssock->param.timeout);
	if (status != PJ_SUCCESS) {
	    ssock->timer.id = TIMER_NONE;
	    status = PJ_SUCCESS;
	}
    }

    status = pj_activesock_start_connect(ssock->asock, pool, remaddr,
					 addr_len);

    if (status == PJ_SUCCESS)
	asock_on_connect_complete(ssock->asock, PJ_SUCCESS);
    else if (status != PJ_EPENDING)
	goto on_error;

    /* Update local address */
    ssock->addr_len = addr_len;
    status = pj_sock_getsockname(ssock->sock, &ssock->local_addr,
				 &ssock->addr_len);
    /* Note that we may not get an IP address here. This can
     * happen for example on Windows, where getsockname()
     * would return 0.0.0.0 if socket has just started the
     * async connect. In this case, just leave the local
     * address with 0.0.0.0 for now; it will be updated
     * once the socket is established.
     */

    /* Update SSL state */
    ssock->is_server = PJ_FALSE;

    return PJ_EPENDING;

on_error:
    reset_ssl_sock_state(ssock);
    return status;
}


PJ_DEF(pj_status_t) pj_ssl_sock_renegotiate(pj_ssl_sock_t *ssock)
{
    int ret;
    pj_status_t status;

    PJ_ASSERT_RETURN(ssock, PJ_EINVAL);

    if (ssock->ssl_state != SSL_STATE_ESTABLISHED) 
	return PJ_EINVALIDOP;

    if (SSL_renegotiate_pending(ssock->ossl_ssl))
	return PJ_EPENDING;

    ret = SSL_renegotiate(ssock->ossl_ssl);
    if (ret <= 0) {
	status = GET_SSL_STATUS(ssock);
    } else {
	status = do_handshake(ssock);
    }

    return status;
}


/* Put back deprecation warning setting */
#if defined(PJ_DARWINOS) && PJ_DARWINOS==1
#  pragma GCC diagnostic pop
#endif


#endif  /* PJ_HAS_SSL_SOCK */

