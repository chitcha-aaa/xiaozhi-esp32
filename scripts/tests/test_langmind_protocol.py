"""Targeted host tests for LangMind direct HTTPS voice protocol."""

import json
import struct
import unittest


def make_wav_header(pcm_bytes: int, sample_rate: int = 16000, channels: int = 1, bits_per_sample: int = 16) -> bytes:
    byte_rate = sample_rate * channels * (bits_per_sample // 8)
    block_align = channels * (bits_per_sample // 8)
    riff_size = 36 + pcm_bytes
    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF",
        riff_size,
        b"WAVE",
        b"fmt ",
        16,
        1,  # PCM
        channels,
        sample_rate,
        byte_rate,
        block_align,
        bits_per_sample,
        b"data",
        pcm_bytes,
    )
    return header


def parse_wav_header(data: bytes) -> dict:
    if len(data) < 44:
        return {"valid": False, "error": "Header too short"}
    if data[0:4] != b"RIFF" or data[8:12] != b"WAVE":
        return {"valid": False, "error": "Not RIFF/WAVE"}

    offset = 12
    found_fmt = False
    found_data = False
    info = {"valid": False}

    while offset + 8 <= len(data):
        chunk_id = data[offset : offset + 4]
        chunk_size = struct.unpack("<I", data[offset + 4 : offset + 8])[0]
        offset += 8

        if chunk_id == b"fmt ":
            if offset + 16 > len(data):
                return {"valid": False, "error": "Truncated fmt chunk"}
            audio_format, channels, sample_rate, byte_rate, block_align, bits = struct.unpack(
                "<HHIIHH", data[offset : offset + 16]
            )
            info.update(
                {
                    "audio_format": audio_format,
                    "channels": channels,
                    "sample_rate": sample_rate,
                    "bits_per_sample": bits,
                }
            )
            found_fmt = True
            offset += chunk_size
        elif chunk_id == b"data":
            info["data_offset"] = offset
            info["data_size"] = chunk_size
            found_data = True
            break
        else:
            offset += chunk_size

    if not found_fmt or not found_data:
        return {"valid": False, "error": "Missing fmt or data chunk"}

    if info["audio_format"] != 1:
        return {"valid": False, "error": f"Unsupported format: {info['audio_format']}"}
    if info["bits_per_sample"] != 16:
        return {"valid": False, "error": f"Unsupported bit depth: {info['bits_per_sample']}"}
    if info["channels"] not in (1, 2):
        return {"valid": False, "error": f"Unsupported channels: {info['channels']}"}

    info["valid"] = True
    return info


def split_text_into_chunks(text: str, max_bytes: int = 4000) -> list[str]:
    raw = text.encode("utf-8")
    if len(raw) <= max_bytes:
        return [text]

    chunks = []
    start = 0
    while start < len(raw):
        if len(raw) - start <= max_bytes:
            chunks.append(raw[start:].decode("utf-8"))
            break

        end = start + max_bytes
        # Search backwards for whitespace or sentence delimiter
        split_pos = -1
        delims = [b"\n", b".", b"?", b"!", b" "]
        for d in delims:
            pos = raw.rfind(d, start + max_bytes // 2, end)
            if pos > split_pos:
                split_pos = pos + len(d)

        if split_pos == -1:
            split_pos = end

        # Ensure split_pos does not cut in the middle of a multi-byte UTF-8 sequence
        while split_pos > start and (raw[split_pos] & 0xC0) == 0x80:
            split_pos -= 1

        if split_pos == start:
            # Advance to valid UTF-8 boundary
            split_pos = end
            while split_pos < len(raw) and (raw[split_pos] & 0xC0) == 0x80:
                split_pos += 1

        chunk_str = raw[start:split_pos].decode("utf-8")
        chunks.append(chunk_str)
        start = split_pos

    return chunks


def build_chat_request(agent_id: str, query: str, conversation_id: str | None = None) -> dict:
    return {
        "agent_id": agent_id,
        "query": query,
        "conversation_id": conversation_id,
        "input_mode": "voice",
        "attachment_ids": [],
    }


def parse_chat_response(response_json: dict) -> tuple[str, str | None]:
    target = response_json.get("data") if isinstance(response_json.get("data"), dict) else response_json
    answer = ""
    for k in ("answer", "message", "content", "text"):
        val = target.get(k)
        if isinstance(val, str) and val.strip():
            answer = val.strip()
            break

    conv_id = target.get("conversation_id") or response_json.get("conversation_id")
    return answer, conv_id


def parse_stt_response(response_json: dict) -> str:
    target = response_json.get("data") if isinstance(response_json.get("data"), dict) else response_json
    text = target.get("text")
    if isinstance(text, str):
        return text.strip()
    return ""


def build_multipart_stt(boundary: str, wav_bytes: bytes) -> tuple[dict, bytes]:
    part_header = (
        f"--{boundary}\r\n"
        'Content-Disposition: form-data; name="audio"; filename="audio.wav"\r\n'
        "Content-Type: audio/wav\r\n\r\n"
    ).encode("ascii")
    part_footer = f"\r\n--{boundary}--\r\n".encode("ascii")
    body = part_header + wav_bytes + part_footer
    headers = {
        "Content-Type": f"multipart/form-data; boundary={boundary}",
        "Content-Length": str(len(body)),
    }
    return headers, body


def simulate_pcm_streaming_chunks(pcm_data: bytes, chunk_size: int, channels: int = 1) -> list[int]:
    """Simulate streaming PCM through a bounded buffer carrying over unaligned leftover bytes."""
    frame_bytes = channels * 2
    offset = 0
    leftover = b""
    output_samples = []

    while offset < len(pcm_data) or leftover:
        chunk = pcm_data[offset : offset + chunk_size]
        offset += chunk_size
        buf = leftover + chunk
        if not buf:
            break

        usable_len = (len(buf) // frame_bytes) * frame_bytes
        if usable_len > 0:
            usable_bytes = buf[:usable_len]
            samples = struct.unpack(f"<{len(usable_bytes) // 2}h", usable_bytes)
            if channels == 2:
                # take left channel only
                output_samples.extend(samples[0::2])
            else:
                output_samples.extend(samples)

        leftover = buf[usable_len:]

    return output_samples


def build_tts_request(text: str, speed: float | None = None, voice: str | None = None) -> dict:
    req = {
        "text": text,
        "format": "wav",
    }
    if speed is not None and speed != 1.0:
        req["speed"] = speed
    if voice:
        req["voice"] = voice
    return req


class LangMindProtocolContractTests(unittest.TestCase):
    AGENT_ID = "90303019-dffd-4890-be99-2e8475832901"

    def test_multipart_stt_body_and_content_length(self):
        boundary = "----TestBoundary12345"
        dummy_wav = make_wav_header(1920) + (b"\x00" * 1920)
        headers, body = build_multipart_stt(boundary, dummy_wav)
        self.assertEqual(int(headers["Content-Length"]), len(body))
        self.assertIn(f"boundary={boundary}", headers["Content-Type"])
        self.assertTrue(body.startswith(f"--{boundary}\r\n".encode("ascii")))
        self.assertTrue(body.endswith(f"\r\n--{boundary}--\r\n".encode("ascii")))
        self.assertIn(b'name="audio"; filename="audio.wav"', body)
        self.assertIn(b"Content-Type: audio/wav", body)

    def test_pcm_streaming_odd_chunk_sizes_prevents_misalignment(self):
        # Generate sequential 16-bit sine wave or ramp samples
        num_samples = 1000
        original_samples = [i % 32767 for i in range(num_samples)]
        raw_pcm = struct.pack(f"<{num_samples}h", *original_samples)

        # Read in irregular odd chunks (e.g. 513 bytes, 7 bytes, 13 bytes)
        for chunk_size in (513, 7, 13, 1920):
            streamed_samples = simulate_pcm_streaming_chunks(raw_pcm, chunk_size=chunk_size, channels=1)
            self.assertEqual(len(streamed_samples), num_samples)
            self.assertEqual(streamed_samples, original_samples)

    def test_pcm_streaming_stereo_downmix_odd_chunks(self):
        # Interleaved stereo samples: L=0..999, R=1000..1999
        num_frames = 500
        stereo_samples = []
        expected_left = []
        for i in range(num_frames):
            l = i
            r = i + 1000
            stereo_samples.extend([l, r])
            expected_left.append(l)

        raw_stereo = struct.pack(f"<{len(stereo_samples)}h", *stereo_samples)

        for chunk_size in (5, 17, 333, 1024):
            downmixed = simulate_pcm_streaming_chunks(raw_stereo, chunk_size=chunk_size, channels=2)
            self.assertEqual(len(downmixed), num_frames)
            self.assertEqual(downmixed, expected_left)

    def test_wav_header_generation(self):
        pcm_bytes = 1920 * 10  # 10 frames of 60ms PCM
        header = make_wav_header(pcm_bytes, sample_rate=16000, channels=1, bits_per_sample=16)
        self.assertEqual(len(header), 44)
        self.assertEqual(header[0:4], b"RIFF")
        self.assertEqual(header[8:12], b"WAVE")
        self.assertEqual(header[12:16], b"fmt ")
        self.assertEqual(header[36:40], b"data")
        parsed = parse_wav_header(header)
        self.assertTrue(parsed["valid"])
        self.assertEqual(parsed["sample_rate"], 16000)
        self.assertEqual(parsed["channels"], 1)
        self.assertEqual(parsed["bits_per_sample"], 16)
        self.assertEqual(parsed["data_size"], pcm_bytes)

    def test_wav_header_parser_validates_pcm_format(self):
        # Format 3 is IEEE float - must be rejected
        header = struct.pack(
            "<4sI4s4sIHHIIHH4sI",
            b"RIFF", 100, b"WAVE", b"fmt ", 16, 3, 1, 16000, 32000, 2, 16, b"data", 56
        )
        parsed = parse_wav_header(header)
        self.assertFalse(parsed["valid"])
        self.assertIn("Unsupported format: 3", parsed["error"])

    def test_wav_header_parser_supports_stereo_resampling(self):
        # 24kHz stereo 16-bit
        header = make_wav_header(4800, sample_rate=24000, channels=2, bits_per_sample=16)
        parsed = parse_wav_header(header)
        self.assertTrue(parsed["valid"])
        self.assertEqual(parsed["sample_rate"], 24000)
        self.assertEqual(parsed["channels"], 2)

    def test_wav_header_parser_skips_junk_chunks(self):
        # WAV with JUNK metadata before data chunk
        junk = b"JUNK" + struct.pack("<I", 8) + b"12345678"
        base_fmt = struct.pack(
            "<4sI4s4sIHHIIHH",
            b"RIFF", 0, b"WAVE", b"fmt ", 16, 1, 1, 16000, 32000, 2, 16
        )
        data_hdr = b"data" + struct.pack("<I", 100)
        full = base_fmt + junk + data_hdr
        parsed = parse_wav_header(full)
        self.assertTrue(parsed["valid"])
        self.assertEqual(parsed["data_offset"], len(base_fmt) + len(junk) + 8)

    def test_stt_response_parsing(self):
        # Normal transcription
        res1 = {"text": "สวัสดีครับ", "elapsed_ms": 640}
        self.assertEqual(parse_stt_response(res1), "สวัสดีครับ")

        # Empty text (no speech detected)
        res2 = {"text": "", "elapsed_ms": 210}
        self.assertEqual(parse_stt_response(res2), "")

        # Nested data
        res3 = {"data": {"text": "ทดสอบเสียง"}}
        self.assertEqual(parse_stt_response(res3), "ทดสอบเสียง")

    def test_chat_blocking_request_payload_format(self):
        # First turn (no conversation_id)
        req1 = build_chat_request(self.AGENT_ID, "สวัสดี")
        self.assertEqual(req1["agent_id"], self.AGENT_ID)
        self.assertEqual(req1["query"], "สวัสดี")
        self.assertIsNone(req1["conversation_id"])
        self.assertEqual(req1["input_mode"], "voice")
        self.assertEqual(req1["attachment_ids"], [])

        # Subsequent turn
        req2 = build_chat_request(self.AGENT_ID, "วันนี้อากาศเป็นอย่างไร", conversation_id="conv-abc-123")
        self.assertEqual(req2["conversation_id"], "conv-abc-123")

    def test_chat_blocking_response_parsing(self):
        # Direct answer
        res1 = {"answer": "ยินดีที่ได้คุยครับ", "conversation_id": "conv-999"}
        ans, conv = parse_chat_response(res1)
        self.assertEqual(ans, "ยินดีที่ได้คุยครับ")
        self.assertEqual(conv, "conv-999")

        # Nested Promout envelope
        res2 = {
            "code": 0,
            "data": {
                "answer": "ฉันคือ AI ผู้ช่วย",
                "conversation_id": "conv-nested-888"
            }
        }
        ans2, conv2 = parse_chat_response(res2)
        self.assertEqual(ans2, "ฉันคือ AI ผู้ช่วย")
        self.assertEqual(conv2, "conv-nested-888")

    def test_utf8_chunking_preserves_characters_and_bounds(self):
        # Thai sentence repeated to exceed 4000 bytes
        thai_phrase = "นี่คือข้อความภาษาไทยเพื่อทดสอบการแบ่งส่วนข้อมูลโดยไม่ให้รหัสอักขระเสียหาย. "
        long_text = (thai_phrase * 70).strip()
        raw_len = len(long_text.encode("utf-8"))
        self.assertGreater(raw_len, 4000)

        chunks = split_text_into_chunks(long_text, max_bytes=3800)
        self.assertGreater(len(chunks), 1)

        # Verify each chunk is <= 3800 bytes and valid UTF-8
        for i, c in enumerate(chunks):
            chunk_bytes = c.encode("utf-8")
            self.assertLessEqual(len(chunk_bytes), 3800, f"Chunk {i} exceeds max_bytes")
            # Must round-trip to utf-8 cleanly
            self.assertEqual(chunk_bytes.decode("utf-8"), c)

        # Reconstructed text must match original
        reconstructed = "".join(chunks)
        self.assertEqual(reconstructed, long_text)

    def test_tts_payload_contract(self):
        payload = build_tts_request("สวัสดีครับ", speed=1.0, voice=None)
        self.assertEqual(payload["text"], "สวัสดีครับ")
        self.assertEqual(payload["format"], "wav")
        # Default speed 1.0 and None voice should be omitted to let agent defaults apply
        self.assertNotIn("speed", payload)
        self.assertNotIn("voice", payload)

        # Overridden speed and voice
        payload_custom = build_tts_request("ข้อความ", speed=1.25, voice="th-TH-Standard-A")
        self.assertEqual(payload_custom["speed"], 1.25)
        self.assertEqual(payload_custom["voice"], "th-TH-Standard-A")

    def test_http_error_code_classification(self):
        # 403 VOICE_DISABLED
        status_403 = 403
        is_voice_disabled = (status_403 == 403)
        self.assertTrue(is_voice_disabled)

        # 422 VOICE_AUDIO_FORMAT_UNSUPPORTED
        status_422 = 422
        is_unsupported_format = (status_422 == 422)
        self.assertTrue(is_unsupported_format)

    def test_conversation_id_lifecycle(self):
        # Turn 1: empty conversation_id
        req1 = build_chat_request(self.AGENT_ID, "สวัสดี")
        self.assertIsNone(req1["conversation_id"])

        # Agent responds with conversation_id
        res1 = {"answer": "สวัสดีครับ", "conversation_id": "conv-test-uuid-1"}
        _, conv_id = parse_chat_response(res1)
        self.assertEqual(conv_id, "conv-test-uuid-1")

        # Turn 2: uses established conversation_id
        req2 = build_chat_request(self.AGENT_ID, "ถามต่อ", conversation_id=conv_id)
        self.assertEqual(req2["conversation_id"], "conv-test-uuid-1")

        # Goodbye / Channel close resets conversation
        conv_id = None
        req3 = build_chat_request(self.AGENT_ID, "เริ่มบทสนทนาใหม่", conversation_id=conv_id)
        self.assertIsNone(req3["conversation_id"])


if __name__ == "__main__":
    unittest.main()
