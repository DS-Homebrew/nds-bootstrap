#include "text.h"
#include "tonccpy.h"

constexpr char16_t map[] =
	u"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"// Special
	u"¡«º»¿           " // Extended punctuation, before ASCII to keep it normal
	u" !\"#$%&'()*+,-./" // Printable ASCII
	u"0123456789:;<=>?"
	u"@ABCDEFGHIJKLMNO"
	u"PQRSTUVWXYZ[\\]^_"
	u"`abcdefghijklmno"
	u"pqrstuvwxyz{|}~ "
	u"ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏ" // Latin extended
	u"ÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞß"
	u"àáâãäåæçèéêëìíîï"
	u"ðñòóôõö÷øùúûüýþÿ"
	u"ĂăĄąĆćĘęĞğİıŁłŃń"
	u"ŐőŚśŞşŰűŹźŻżȘșȚț"
	u"ΐΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟ" // Greek
	u"ΠΡ ΣΤΥΦΧΨΩΪΫάέήί"
	u"ΰαβγδεζηθικλμνξο"
	u"πρςστυφχψωϊϋόύώϏ"
	u"ЀЁЂЃЄЅІЇЈЉЊЋЌЍЎЏ" // Cyrillic
	u"АБВГДЕЖЗИЙКЛМНОП"
	u"РСТУФХЦЧШЩЪЫЬЭЮЯ"
	u"абвгдежзийклмноп"
	u"рстуфхцчшщъыьэюя"
	u"ѐёђѓєѕіїјљњћќѝўџ"
	u"אבגדהוזחטיךכלםמן" // Hebrew
	u"נסעףפץצקרשתװױײ׳״"
	u"ءآأؤإئابةتثجحخدذ" // Arabic
	u"رزسشصضطظعغـفقكلم"
	u"نهوىيﯨﯩﺂﺄﺆﺈﺊﺋﺌﺎﺐ"
	u"ﺑﺒﺔﺖﺗﺘﺚﺛﺜﺞﺟﺠﺢﺣﺤﺦ"
	u"ﺧﺨﺪﺬﺮﺰﺲﺳﺴﺶﺷﺸﺺﺻﺼﺾ"
	u"ﺿﻀﻂﻃﻄﻆﻇﻈﻊﻋﻌﻎﻏﻐﻒﻓ"
	u"ﻔﻖﻗﻘﻚﻛﻜﻞﻟﻠﻢﻣﻤﻦﻧﻨ"
	u"ﻪﻫﻬﻮﻰﻲﻳﻴ،؟      "
	u"　｡｢｣､･ｦｧｨｩｪｫｬｭｮｯ" // Kana
	u"ｰｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿ"
	u"ﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏ"
	u"ﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜﾝﾞﾟ"
	u"主位儲关出到动動区器回图圖地址块" // Hànzì
	u"增定底开強强戏截戲择擇数數时時查"
	u"檢游率界畫目看移端置自至視計設跳"
	u"轉转返退选遊選部重鐘钟開關離面頂"
	u"頻顶项频"
	      u"가게기끔덤도돌동래럭로료" // Hangul
	u"리린메면뱅뷰상샷선설셋소속스아어"
	u"위으이인임자정종주켬크클택프향화";

constexpr char16_t arabicPresentationForms[][3] = {
	// Initial, Medial, Final
	{u'آ', u'ﺂ', u'ﺂ'}, // Alef with madda above
	{u'أ', u'ﺄ', u'ﺄ'}, // Alef with hamza above
	{u'ؤ', u'ﺆ', u'ﺆ'}, // Waw with hamza above
	{u'إ', u'ﺈ', u'ﺈ'}, // Alef with hamza below
	{u'ﺋ', u'ﺌ', u'ﺊ'}, // Yeh with hamza above
	{u'ا', u'ﺎ', u'ﺎ'}, // Alef
	{u'ﺑ', u'ﺒ', u'ﺐ'}, // Beh
	{u'ة', u'ﺔ', u'ﺔ'}, // Teh marbuta
	{u'ﺗ', u'ﺘ', u'ﺖ'}, // Teh
	{u'ﺛ', u'ﺜ', u'ﺚ'}, // Theh
	{u'ﺟ', u'ﺠ', u'ﺞ'}, // Jeem
	{u'ﺣ', u'ﺤ', u'ﺢ'}, // Hah
	{u'ﺧ', u'ﺨ', u'ﺦ'}, // Khah
	{u'د', u'ﺪ', u'ﺪ'}, // Dal
	{u'ذ', u'ﺬ', u'ﺬ'}, // Thal
	{u'ر', u'ﺮ', u'ﺮ'}, // Reh
	{u'ز', u'ﺰ', u'ﺰ'}, // Zain
	{u'ﺳ', u'ﺴ', u'ﺲ'}, // Seen
	{u'ﺷ', u'ﺸ', u'ﺶ'}, // Sheen
	{u'ﺻ', u'ﺼ', u'ﺺ'}, // Sad
	{u'ﺿ', u'ﻀ', u'ﺾ'}, // Dad
	{u'ﻃ', u'ﻄ', u'ﻂ'}, // Tah
	{u'ﻇ', u'ﻈ', u'ﻆ'}, // Zah
	{u'ﻋ', u'ﻌ', u'ﻊ'}, // Ain
	{u'ﻏ', u'ﻐ', u'ﻎ'}, // Ghain
	{u'ػ', u'ػ', u'ػ'}, // Keheh with two dots above
	{u'ؼ', u'ؼ', u'ؼ'}, // Keheh with three dots below
	{u'ؽ', u'ؽ', u'ؽ'}, // Farsi yeh with inverted v
	{u'ؾ', u'ؾ', u'ؾ'}, // Farsi yeh with two dots above
	{u'ؿ', u'ؿ', u'ؿ'}, // Farsi yeh with three docs above
	{u'ـ', u'ـ', u'ـ'}, // Tatweel
	{u'ﻓ', u'ﻔ', u'ﻒ'}, // Feh
	{u'ﻗ', u'ﻘ', u'ﻖ'}, // Qaf
	{u'ﻛ', u'ﻜ', u'ﻚ'}, // Kaf
	{u'ﻟ', u'ﻠ', u'ﻞ'}, // Lam
	{u'ﻣ', u'ﻤ', u'ﻢ'}, // Meem
	{u'ﻧ', u'ﻨ', u'ﻦ'}, // Noon
	{u'ﻫ', u'ﻬ', u'ﻪ'}, // Heh
	{u'و', u'ﻮ', u'ﻮ'}, // Waw
	{u'ﯨ', u'ﯩ', u'ﻰ'}, // Alef maksura
	{u'ﻳ', u'ﻴ', u'ﻲ'}, // Yeh
};

// Can't do a binary search as the map isn't fully sorted (so the image looks better)
u16 getIndex(char16_t c) {
	for(uint i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		if(c == map[i]) {
			return i;
		}
	}

	return c == '?' ? '?' : getIndex('?');
}

// Specifically the Arabic letters that have supported presentation forms
bool isArabic(char16_t c) {
	return c >= 0x0622 && c <= 0x064A;
}

bool isStrongRTL(char16_t c) {
	// Hebrew, Arabic, or RLM
	return (c >= 0x0590 && c <= 0x05FF) || (c >= 0x0600 && c <= 0x06FF) || c == 0x200F;
}

bool isWeak(char16_t c) {
	return c < 'A' || (c > 'Z' && c < 'a') || (c > 'z' && c < 127);
}

bool isNumber(char16_t c) {
	return c >= '0' && c <= '9';
}

char16_t arabicForm(char16_t current, char16_t prev, char16_t next) {
	if(isArabic(current)) {
		// If previous should be connected to
		if((prev >= 0x626 && prev <= 0x62E && prev != 0x627 && prev != 0x629) || (prev >= 0x633 && prev <= 0x64A && prev != 0x648)) {
			if(isArabic(next)) // If next is arabic, medial
				return arabicPresentationForms[current - 0x622][1];
			else // If not, final
				return arabicPresentationForms[current - 0x622][2];
		} else {
			if(isArabic(next)) // If next is arabic, initial
				return arabicPresentationForms[current - 0x622][0];
			else // If not, isolated
				return current;
		}
	}

	return current;
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
		} else if(rtl && !isStrongRTL(map[*p]) && (!isWeak(map[*p]) || isNumber(map[*p]))) {
			// Save where we are as the end of the LTR section
			ltrEnd = p + 1;

			// Go back until an RTL character or the start of the string
			bool allNumbers = true;
			while(!isStrongRTL(map[*p]) && p != begin) {
				// Check for if the LTR section is only numbers,
				// if so they won't be removed from the end
				if(allNumbers && !isNumber(map[*p]) && !isWeak(map[*p]))
					allNumbers = false;
				p--;
			}

			// Save where we are to return to after printing the LTR section
			ltrBegin = p;

			// If on an RTL char right now, add one
			if(isStrongRTL(map[*p])) {
				p++;
			}

			// Remove all punctuation and, if the section isn't only numbers,
			// numbers from the end of the LTR section
			if(allNumbers) {
				while(isWeak(map[*p]) && !isNumber(map[*p])) {
					if(p != begin)
						ltrBegin++;
					p++;
				}
			} else {
				while(isWeak(map[*p])) {
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
			switch(map[*p]) {
				case '(':
					*(res++) = getIndex(')');
					break;
				case ')':
					*(res++) = getIndex('(');
					break;
				case '[':
					*(res++) = getIndex(']');
					break;
				case ']':
					*(res++) = getIndex('[');
					break;
				case '<':
					*(res++) = getIndex('>');
					break;
				case '>':
					*(res++) = getIndex('<');
					break;
				default:
					*(res++) = getIndex(arabicForm(map[*p], p > begin ? map[*(p - 1)] : 0, p < end - 1 ? map[*(p + 1)] : 0));
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
		*(dst++) = getIndex(codepoint);

		if((codepoint >= 0x0590 && codepoint <= 0x05FF) || (codepoint >= 0x0600 && codepoint <= 0x06FF) || codepoint == 0x200F)
			containsRTL = true;
	}

	// Null terminator
	*dst = 0;

	if(containsRTL)
		processRTL(dstStart, dst);
}
