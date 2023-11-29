#ifndef SHA256_H
#define SHA256_H

#include <string>
#include <array>

class SHA256 {

public:
	SHA256();
	void update(const uint8_t * data, size_t length);
	void update(const std::string &data);
	uint8_t * digest();

	static std::string toString(const uint8_t * digest);

private:
	uint8_t  m_data[64];
	uint32_t m_blocklen;
	uint64_t m_bitlen;
	uint32_t m_state[8]; //A, B, C, D, E, F, G, H

	static uint32_t rotr(uint32_t x, uint32_t n);
	static uint32_t choose(uint32_t e, uint32_t f, uint32_t g);
	static uint32_t majority(uint32_t a, uint32_t b, uint32_t c);
	static uint32_t sig0(uint32_t x);
	static uint32_t sig1(uint32_t x);
	void transform();
	void pad();
	void revert(uint8_t * hash);
};

// extern "C" {
//     SHA256* SHA256_new(){ return new SHA256(); }
// 	void update_ext(SHA256* sha, const uint8_t * data, size_t length){ sha->update(data, length); }
// 	uint8_t * digest_ext(SHA256* sha){ return sha->digest(); }
// 	const char * toString_ext(SHA256* sha, const uint8_t * digest){ return sha->toString(digest).c_str(); }

// }

#endif
