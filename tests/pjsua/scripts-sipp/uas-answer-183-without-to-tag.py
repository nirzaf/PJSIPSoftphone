# $Id: uas-answer-183-without-to-tag.py 5844 2018-07-26 03:37:08Z nanang $
#
import inc_const as const

PJSUA = ["--null-audio --max-calls=1 --no-tcp $SIPP_URI"]

PJSUA_EXPECTS = [[0, "Audio updated", ""]]
