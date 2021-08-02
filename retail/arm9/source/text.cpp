#include "text.h"
#include "tonccpy.h"

const char16_t hanzi[] = u"主位儲关出到动動区器回图圖地址块增定底开強强戏截戲择擇数數时時查檢游率界畫目看移端置自至視計設跳轉转返退选遊選部重鐘钟開關離面頂頻顶项频";
const char16_t hangul[] = u"가게기끔덤도돌동래럭로료리린메면뱅뷰상샷선설셋소속스아어위으이인임자정종주켬크크클택프향화";

bool isStrongRTL(u16 c) {
	return c >= 0x180 && c < 0x200;
}

bool isWeak(u16 c) {
	return c < 'A' || (c > 'Z' && c < 'a') || (c > 'z' && c < 127);
}

bool isNumber(u16 c) {
	return c >= '0' && c <= '9';
}

void processRTL(u16 *begin, u16 *end) {
	u16 *ltrBegin = end, *ltrEnd = end;
	bool rtl = true;
	u16 buffer[end - begin + 1] = {0};
	u16 *res = buffer;

	// Loop through string and save to buffer
	for(u16 *p = end - 1; true; p += (rtl ? -1 : 1)) {
		// If we hit the end of the string in an LTR section of an RTL
		// string, it may not be done, if so jump back to printing RTL
		if(p == (rtl ? begin - 1 : end)) {
			if(ltrBegin == end || (ltrBegin == begin && ltrEnd == end)) {
				break;
			} else {
				p = ltrBegin;
				ltrBegin = end;
				rtl = true;
			}
		}

		// If at the end of an LTR section within RTL, jump back to the RTL
		if(p == ltrEnd && ltrBegin != end) {
			if(ltrBegin == begin && (!isWeak(*ltrBegin) || isNumber(*ltrBegin)))
				break;

			p = ltrBegin;
			ltrBegin = end;
			rtl = true;
		// If in RTL and hit a non-RTL character that's not punctuation, switch to LTR
		} else if(rtl && !isStrongRTL(*p) && (!isWeak(*p) || isNumber(*p))) {
			// Save where we are as the end of the LTR section
			ltrEnd = p + 1;

			// Go back until an RTL character or the start of the string
			bool allNumbers = true;
			while(!isStrongRTL(*p) && p != begin) {
				// Check for if the LTR section is only numbers,
				// if so they won't be removed from the end
				if(allNumbers && !isNumber(*p) && !isWeak(*p))
					allNumbers = false;
				p--;
			}

			// Save where we are to return to after printing the LTR section
			ltrBegin = p;

			// If on an RTL char right now, add one
			if(isStrongRTL(*p)) {
				p++;
			}

			// Remove all punctuation and, if the section isn't only numbers,
			// numbers from the end of the LTR section
			if(allNumbers) {
				while(isWeak(*p) && !isNumber(*p)) {
					if(p != begin)
						ltrBegin++;
					p++;
				}
			} else {
				while(isWeak(*p)) {
					if(p != begin)
						ltrBegin++;
					p++;
				}
			}

			// But then allow all numbers directly touching the strong LTR or with 1 weak between
			while((p - 1 >= begin && isNumber(*(p - 1))) || (p - 2 >= begin && isWeak(*(p - 1)) && isNumber(*(p - 2)))) {
				if(p - 1 != begin)
					ltrBegin--;
				p--;
			}

			rtl = false;
		}

		// Brackets are flipped in RTL
		if(rtl) {
			switch(*p) {
				case '(':
					*(res++) = ')';
					break;
				case ')':
					*(res++) = '(';
					break;
				case '[':
					*(res++) = ']';
					break;
				case ']':
					*(res++) = '[';
					break;
				case '<':
					*(res++) = '>';
					break;
				case '>':
					*(res++) = '<';
					break;
				default:
					*(res++) = *p;
					break;
			}
		} else {
			*(res++) = *p;
		}
	}

	tonccpy(begin, buffer, sizeof(buffer));
}

void setIgmString(const char *src, u16 *dst) {
	bool containsRTL = false;
	u16 *dstStart = dst;

	while(*src) {
		// Get codepoint from UTF-8
		u16 codepoint = 0;
		if((*src & 0x80) == 0) {
			codepoint = *(src++);
		} else if((*src & 0xE0) == 0xC0) {
			codepoint = ((*src++ & 0x1F) << 6);
			if((*src & 0xC0) == 0x80) codepoint |= *src++ & 0x3F;
		} else if((*src & 0xF0) == 0xE0) {
			codepoint = (*src++ & 0xF) << 12;
			if((*src & 0xC0) == 0x80) codepoint |= (*src++ & 0x3F) << 6;
			if((*src & 0xC0) == 0x80) codepoint |=  *src++ & 0x3F;
		} else if((*src & 0xF8) == 0xF0) {
			codepoint = (*src++ & 0x7) << 18;
			if((*src & 0xC0) == 0x80) codepoint |= (*src++ & 0x3F) << 12;
			if((*src & 0xC0) == 0x80) codepoint |= (*src++ & 0x3F) << 6;
			if((*src & 0xC0) == 0x80) codepoint |=  *src++ & 0x3F;
		} else {
			// Character isn't valid, use ?
			src++;
			codepoint = '?';
		}

		// Add to string
		if(codepoint < 127) { // ASCII
			*(dst++) = codepoint;
		} else if(codepoint > 127 && codepoint < 0x250) { // Extended Latin
			if(codepoint <= u'¿') { // Symbols at the Start of Latin-1 Supplement
				*(dst++) = codepoint - 0xA0;
			} else if(codepoint <= u'ğ') { // Latin-1 Supplement + start of Latin Extended-A
				*(dst++) = codepoint;
			} else if(codepoint <= u'ı') { // İ and ı
				*(dst++) = codepoint - 0x30;
			} else if(codepoint <= u'ń') { // Ł, ł, Ń, and ń
				*(dst++) = codepoint - 0x38;
			} else if(codepoint <= u'ş') { // Ő, ő, Ś, ś, Ş, and ş
				*(dst++) = codepoint - 0x4E;
			} else if(codepoint <= u'ű') { // Ű and ű
				*(dst++) = codepoint - 0x6E;
			} else if(codepoint <= u'ż') { // Ź, ź, Ż, and ż
				*(dst++) = codepoint - 0x77;
			} else if(codepoint <= u'ș') { // Latin Extended-B: Ș and ș
				*(dst++) = codepoint - 0x102;
			}
		} else if(codepoint >= u'｡' && codepoint <= u'ﾟ') { // Half-width Katakana
			*(dst++) = codepoint - 0xFF60 + 0x80;
		} else if(codepoint >= u'ΐ' && codepoint <= u'Ϗ') { // Greek
			*(dst++) = codepoint - 0x390 + 0x120;
		} else if(codepoint >= u'Ѐ' && codepoint <= u'џ') { // Cyrillic
			*(dst++) = codepoint - 0x400 + 0x160;
		} else if(codepoint >= u'א' && codepoint <= u'״') { // Hebrew
			containsRTL = true;
			if(codepoint <= u'ת') // Letters
				*(dst++) = codepoint - 0x5D0 + 0x1C0;
			else // Punctuation and Ligatures
				*(dst++) = codepoint - 0x5F0 + 0x1DB;
		} else if(codepoint >= hanzi[0] && codepoint <= hanzi[sizeof(hanzi) / sizeof(hanzi[0]) - 2]) { // Hànzì
			for(uint i = 0; i < sizeof(hanzi) / sizeof(hanzi[0]); i++) {
				if(codepoint == hanzi[i]) {
					*(dst++) = 0x1E0 + i;
					break;
				}
			}
		} else if(codepoint >= hangul[0] && codepoint <= hangul[sizeof(hangul) / sizeof(hangul[0]) - 2]) { // Hangul
			for(uint i = 0; i < sizeof(hangul) / sizeof(hangul[0]); i++) {
				if(codepoint == hangul[i]) {
					*(dst++) = 0x21B + i;
					break;
				}
			}
		} else { // Unsupported
			*(dst++) = '?';
		}
	}

	// Null terminator
	*dst = 0;

	if(containsRTL)
		processRTL(dstStart, dst);
}
