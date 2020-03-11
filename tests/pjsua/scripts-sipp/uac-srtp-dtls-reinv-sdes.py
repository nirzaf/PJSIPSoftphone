# $Id: uac-srtp-dtls-reinv-sdes.py 5855 2018-08-15 02:44:53Z nanang $
#
import inc_const as const

PJSUA = ["--null-audio --max-calls=1 --auto-answer=200 --no-tcp --srtp-secure 0 --use-srtp 2 --srtp-keying=1"]

PJSUA_EXPECTS = [[0, "SRTP uses keying method DTLS-SRTP", ""],
		 [0, "SRTP uses keying method SDES", ""]
		]
