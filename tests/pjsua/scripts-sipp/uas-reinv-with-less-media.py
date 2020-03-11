# $Id: uas-reinv-with-less-media.py 5844 2018-07-26 03:37:08Z nanang $
#
import inc_const as const

PJSUA = ["--null-audio --extra-audio --max-calls=1 --no-tcp $SIPP_URI"]

# Send hold after remote holds (double hold)
PJSUA_EXPECTS = [[0, const.MEDIA_HOLD, ""],
		 [0, "ACK sip:", "H"]
		]
