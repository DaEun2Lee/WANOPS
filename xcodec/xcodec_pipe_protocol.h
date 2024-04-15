/*
 * Copyright (c) 2010-2014 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	XCODEC_XCODEC_PIPE_PROTOCOL_H
#define	XCODEC_XCODEC_PIPE_PROTOCOL_H

/*
 * Usage:
 * 	<OP_HELLO> length[uint8_t] data[uint8_t x length]
 *
 * Effects:
 * 	Must appear at the start of and only at the start of an encoded	stream.
 *
 * Sife-effects:
 * 	Possibly many.
 */
#define	XCODEC_PIPE_OP_HELLO	((uint8_t)0xff)

/*
 * Usage:
 * 	<OP_LEARN> count[uint16_t] data[[uint8_t x XCODEC_PIPE_SEGMENT_LENGTH] x count]
 *
 * Effects:
 * 	The each `data' is hashed, the hash is associated with the data if possible.
 *
 * Side-effects:
 * 	None.
 */
#define	XCODEC_PIPE_OP_LEARN	((uint8_t)0xf1)

/*
 * Usage:
 * 	<OP_ASK> count[uint16_t] hash[uint64_t x count]
 *
 * Effects:
 * 	An OP_LEARN will be sent in response with the data corresponding to the
 * 	requested hashes.
 *
 * 	If any hash is unknown, error will be indicated.
 *
 * Side-effects:
 * 	None.
 */
#define	XCODEC_PIPE_OP_ASK	((uint8_t)0xf0)

/*
 * Usage:
 * 	<OP_EOS>
 *
 * Effects:
 * 	Alert the other party that we have no intention of sending more data.
 *
 * Side-effects:
 * 	The other party will send <OP_EOS_ACK> when it has processed all of
 * 	the data we have sent.
 */
#define	XCODEC_PIPE_OP_EOS	((uint8_t)0xfc)

/*
 * Usage:
 * 	<OP_EOS_ACK>
 *
 * Effects:
 * 	Alert the other party that we have no intention of reading more data.
 *
 * Side-effects:
 * 	The connection will be torn down.
 */
#define	XCODEC_PIPE_OP_EOS_ACK	((uint8_t)0xfb)

/*
 * Usage:
 * 	<OP_FRAME> length[uint32_t] data[uint8_t x length]
 *
 * Effects:
 * 	Frames an encoded chunk.
 *
 * Side-effects:
 * 	The other party will send <OP_ADVANCE> when it has processed one or
 * 	more frames successfully in their entirety.
 */
#define	XCODEC_PIPE_OP_FRAME	((uint8_t)0x02)

/*
 * Usage:
 * 	<OP_ADVANCE> count[uint32_t]
 *
 * Effects:
 * 	The other party is informed that we have processed the last `count'
 * 	frames, and that it need not retain segments referenced within them
 * 	in case an ASK is generated by the encoder.
 *
 * Side-effects:
 * 	None.
 */
#define	XCODEC_PIPE_OP_ADVANCE	((uint8_t)0x01)

#endif /* !XCODEC_XCODEC_PIPE_PROTOCOL_H */
