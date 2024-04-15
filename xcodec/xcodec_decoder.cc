/*
 * Copyright (c) 2008-2013 Juli Mallett. All rights reserved.
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

#include <common/buffer.h>
#include <common/endian.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>
#include <xcodec/xcodec_decoder.h>
#include <xcodec/xcodec_encoder.h>
#include <xcodec/xcodec_hash.h>

XCodecDecoder::XCodecDecoder(XCodecCache *cache)
: log_("/xcodec/decoder"),
  cache_(cache),
  window_()
{ }

XCodecDecoder::~XCodecDecoder()
{ }

/*
 * XXX These comments are out-of-date.
 *
 * Decode an XCodec-encoded stream.  Returns false if there was an
 * inconsistency, error or unrecoverable condition in the stream.
 * Returns true if we were able to process the stream entirely or
 * expect to be able to finish processing it once more data arrives.
 * The input buffer is cleared of anything we can parse right now.
 *
 * Since some events later in the stream (i.e. ASK or LEARN) may need
 * to be processed before some earlier in the stream (i.e. REF), we
 * parse the stream into a list of actions to take, performing them
 * as we go if possible, otherwise queueing them to occur until the
 * action that is blocking the stream has been satisfied or the stream
 * has been closed.
 *
 * XXX For now we will ASK in every stream where an unknown hash has
 * occurred and expect a LEARN in all of them.  In the future, it is
 * desirable to optimize this.  Especially once we start putting an
 * instance UUID in the HELLO message and can tell which streams
 * share an originator.
 */
bool
XCodecDecoder::decode(Buffer *output, Buffer *input, std::set<uint64_t>& unknown_hashes)
{
	while (!input->empty()) {
		size_t off;
		if (!input->find(XCODEC_MAGIC, &off)) {
			input->moveout(output);
			break;
		}

		if (off != 0) {
			output->append(input, off);
			input->skip(off);
		}
		ASSERT(log_, !input->empty());

		/*
		 * Need the following byte at least.
		 */
		if (input->length() == 1)
			break;

		uint8_t op;
		input->extract(&op, sizeof XCODEC_MAGIC);

		switch (op) {
		case XCODEC_OP_ESCAPE:
			output->append(XCODEC_MAGIC);
			input->skip(sizeof XCODEC_MAGIC + sizeof op);
			break;
		case XCODEC_OP_EXTRACT:
			if (input->length() < sizeof XCODEC_MAGIC + sizeof op + XCODEC_SEGMENT_LENGTH)
				goto done;
			else {
				input->skip(sizeof XCODEC_MAGIC + sizeof op);

				BufferSegment *seg;
				input->copyout(&seg, XCODEC_SEGMENT_LENGTH);
				input->skip(XCODEC_SEGMENT_LENGTH);

				uint64_t hash = XCodecHash::hash(seg->data());
				BufferSegment *oseg = cache_->lookup(hash);
				if (oseg != NULL) {
					if (oseg->equal(seg)) {
						seg->unref();
						seg = oseg;
					} else {
						/*
						 * Now that we allow clamping of size,
						 * we can encounter name reuse.  We should
						 * in this case simply replace the old
						 * name with this one.  With out-of-band
						 * caches, we can't allow name reuse as it
						 * might corrupt old data, but we do not
						 * use <EXTRACT> in out-of-band encoders
						 * for precisely that reason.
						 *
						 * Races are possible here, which is why
						 * it is important that the protocol which
						 * uses XCodec is able to do some kind of
						 * framing so that it can verify decoded
						 * data, and refetch data from the peer if
						 * the decoded data is incorrect.
						 */
						INFO(log_) << "Name reuse in <EXTRACT>.";
						oseg->unref();
						cache_->replace(hash, seg);
					}
				} else {
					cache_->enter(hash, seg);
				}

				window_.declare(hash, seg);
				output->append(seg);
				seg->unref();
			}
			break;
		case XCODEC_OP_REF:
			if (input->length() < sizeof XCODEC_MAGIC + sizeof op + sizeof (uint64_t))
				goto done;
			else {
				uint64_t behash;
				input->extract(&behash, sizeof XCODEC_MAGIC + sizeof op);
				uint64_t hash = BigEndian::decode(behash);

				BufferSegment *oseg = cache_->lookup(hash);
				if (oseg == NULL) {
					decode_skim(input, unknown_hashes);
					DEBUG(log_) << "Sending <ASK>, waiting for <LEARN>.";
					return (true);
				}

				input->skip(sizeof XCODEC_MAGIC + sizeof op + sizeof behash);

				window_.declare(hash, oseg);
				output->append(oseg);
				oseg->unref();
			}
			break;
		case XCODEC_OP_BACKREF:
			if (input->length() < sizeof XCODEC_MAGIC + sizeof op + sizeof (uint8_t))
				goto done;
			else {
				uint8_t idx;
				input->moveout(&idx, sizeof XCODEC_MAGIC + sizeof op, sizeof idx);

				BufferSegment *oseg = window_.dereference(idx);
				if (oseg == NULL) {
					ERROR(log_) << "Index not present in <BACKREF> window: " << (unsigned)idx;
					return (false);
				}

				output->append(oseg);
				oseg->unref();
			}
			break;
		default:
			ERROR(log_) << "Unsupported XCodec opcode " << (unsigned)op << ".";
			return (false);
		}
	}
done:	return (true);
}

/*
 * We have encountered an unknown hash; skim through the rest of the
 * stream and identify any other unresolvable references, so that we
 * can properly interrogate the peer for as many as possible at once
 * rather than having to go one-by-one and slowly.
 */
void
XCodecDecoder::decode_skim(const Buffer *resid, std::set<uint64_t>& unknown_hashes)
{
	std::set<uint64_t> defined_hashes;

	Buffer input;
	input.append(resid);
	while (!input.empty()) {
		size_t off;
		if (!input.find(XCODEC_MAGIC, &off))
			break;

		if (off != 0)
			input.skip(off);
		ASSERT(log_, !input.empty());

		/*
		 * Need the following byte at least.
		 */
		if (input.length() == 1)
			break;

		uint8_t op;
		input.extract(&op, sizeof XCODEC_MAGIC);

		switch (op) {
		case XCODEC_OP_ESCAPE:
			input.skip(sizeof XCODEC_MAGIC + sizeof op);
			break;
		case XCODEC_OP_EXTRACT:
			if (input.length() < sizeof XCODEC_MAGIC + sizeof op + XCODEC_SEGMENT_LENGTH)
				return;
			else {
				input.skip(sizeof XCODEC_MAGIC + sizeof op);

				BufferSegment *seg;
				input.copyout(&seg, XCODEC_SEGMENT_LENGTH);
				input.skip(XCODEC_SEGMENT_LENGTH);

				uint64_t hash = XCodecHash::hash(seg->data());
				if (defined_hashes.find(hash) == defined_hashes.end())
					defined_hashes.insert(hash);
				seg->unref();
			}
			break;
		case XCODEC_OP_REF:
			if (input.length() < sizeof XCODEC_MAGIC + sizeof op + sizeof (uint64_t))
				return;
			else {
				uint64_t behash;
				input.extract(&behash, sizeof XCODEC_MAGIC + sizeof op);
				uint64_t hash = BigEndian::decode(behash);

				BufferSegment *oseg = cache_->lookup(hash);
				if (oseg == NULL) {
					if (defined_hashes.find(hash) == defined_hashes.end() &&
					    unknown_hashes.find(hash) == unknown_hashes.end())
						unknown_hashes.insert(hash);
				} else {
					oseg->unref();
				}

				input.skip(sizeof XCODEC_MAGIC + sizeof op + sizeof behash);
			}
			break;
		case XCODEC_OP_BACKREF:
			if (input.length() < sizeof XCODEC_MAGIC + sizeof op + sizeof (uint8_t))
				return;
			else
				input.skip(sizeof XCODEC_MAGIC + sizeof op + sizeof (uint8_t));
			break;
		default:
			ERROR(log_) << "Unsupported XCodec opcode when skimming " << (unsigned)op << ".";
			return;
		}
	}
}
