#include "text.h"
#include "tonccpy.h"

#include <array>
#include <map>

IgmFont extendedFont = IgmFont::extendedLatin;

constexpr char16_t mapAscii[] =
	u"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
	u"ẲẴẪỶỸỴ\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
	u" !\"#$%&'()*+,-./"
	u"0123456789:;<=>?"
	u"@ABCDEFGHIJKLMNO"
	u"PQRSTUVWXYZ[\\]^_"
	u"`abcdefghijklmno"
	u"pqrstuvwxyz{|}~\x7F";

constexpr char16_t mapArabic[] =
	u"ءآأؤإئابةتثجحخدذ"
	u"رزسشصضطظعغـفقكلم"
	u"نهوىيﯨﯩﺂﺄﺆﺈﺊﺋﺌﺎﺐ"
	u"ﺑﺒﺔﺖﺗﺘﺚﺛﺜﺞﺟﺠﺢﺣﺤﺦ"
	u"ﺧﺨﺪﺬﺮﺰﺲﺳﺴﺶﺷﺸﺺﺻﺼﺾ"
	u"ﺿﻀﻂﻃﻄﻆﻇﻈﻊﻋﻌﻎﻏﻐﻒﻓ"
	u"ﻔﻖﻗﻘﻚﻛﻜﻞﻟﻠﻢﻣﻤﻦﻧﻨ"
	u"ﻪﻫﻬﻮﻰﻲﻳﻴ،؟ﻻﻼ";

constexpr char16_t mapChinese[] =
	u"主书亮位儲出到动動区器回图圖地址"
	u"块存定屏幕底度式戏截戲择擇数數时"
	u"明显時書查模檢游率界畫目看移端置"
	u"自至螢視計設說说跳轉转返退选遊選"
	u"部重量鐘钟開離面音頂頻顶项频　";

constexpr char16_t mapCyrillic[] =
	u"ЂЃ ѓ      Љ ЊЌЋЏ"
	u"ђ         љ њќћџ"
	u" ЎўЈ Ґ  Ё Є    Ї"
	u"  Ііґ   ё є јЅѕї"
	u"АБВГДЕЖЗИЙКЛМНОП"
	u"РСТУФХЦЧШЩЪЫЬЭЮЯ"
	u"абвгдежзийклмноп"
	u"рстуфхцчшщъыьэюя";

constexpr char16_t mapExtendedLatin[] =
	u"ĂăĄąĆćĘęĞğİıŁłŃń"
	u"ŐőŚśŞşŰűŹźŻżȘșȚț"
	u" ¡   …     «    "
	u"          º»   ¿"
	u"ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏ"
	u"ÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞß"
	u"àáâãäåæçèéêëìíîï"
	u"ðñòóôõö÷øùúûüýþÿ";

constexpr char16_t mapGreek[] =
	u"                "
	u"                "
	u"  Ά             "
	u"        ΈΉΊ Ό ΎΏ"
	u"ΐΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟ"
	u"ΠΡ ΣΤΥΦΧΨΩΪΫάέήί"
	u"ΰαβγδεζηθικλμνξο"
	u"πρςστυφχψωϊϋόύώ";

constexpr char16_t mapHangul[] =
	u"가게기덤도돌동드래럭로료리린메면"
	u"모뱅뷰샷선설셋소속스아어운위으이"
	u"인임자정종주카크클택트프화";

constexpr char16_t mapHebrew[] =
	u"                "
	u"                "
	u"                "
	u"                "
	u"                "
	u"    װױײ׳״       "
	u"אבגדהוזחטיךכלםמן"
	u"נסעףפץצקרשת";

constexpr char16_t mapJapanese[] =
	u"‥…　いぅかさどなにぬのむるをん"
	u"アクゲシジスセダットドバビプムモ"
	u"ャュョリレロンー　　　　　　　　"
	u"上下主了動定度戻択数明書画終自設"
	u"説速選量面音";

constexpr char16_t mapVietnamese[] =
	u"ẠẮẰẶẤẦẨẬẼẸẾỀỂỄỆỐ"
	u"ỒỔỖỘỢỚỜỞỊỎỌỈỦŨỤỲ"
	u"Õắằặấầẩậẽẹếềểễệố"
	u"ồổỗỠƠộờởịỰỨỪỬơớƯ"
	u"ÀÁÂÃẢĂẳẵÈÉÊẺÌÍĨỳ"
	u"ĐứÒÓÔạỷừửÙÚỹỵÝỡư"
	u"àáâãảăữẫèéêẻìíĩỉ"
	u"đựòóôõỏọụùúũủýợỮ";


constexpr const char16_t *extendedMaps[] = {
	mapArabic,
	mapChinese,
	mapCyrillic,
	mapExtendedLatin,
	mapGreek,
	mapHangul,
	mapHebrew,
	mapJapanese,
	mapVietnamese
};

std::map<char16_t, std::array<char16_t, 3>> arabicPresentationForms = {
	// Initial, Medial, Final
	{u'آ', {u'آ', u'ﺂ', u'ﺂ'}}, // Alef with madda above
	{u'أ', {u'أ', u'ﺄ', u'ﺄ'}}, // Alef with hamza above
	{u'ؤ', {u'ؤ', u'ﺆ', u'ﺆ'}}, // Waw with hamza above
	{u'إ', {u'إ', u'ﺈ', u'ﺈ'}}, // Alef with hamza below
	{u'ئ', {u'ﺋ', u'ﺌ', u'ﺊ'}}, // Yeh with hamza above
	{u'ا', {u'ا', u'ﺎ', u'ﺎ'}}, // Alef
	{u'ب', {u'ﺑ', u'ﺒ', u'ﺐ'}}, // Beh
	{u'ة', {u'ة', u'ﺔ', u'ﺔ'}}, // Teh marbuta
	{u'ت', {u'ﺗ', u'ﺘ', u'ﺖ'}}, // Teh
	{u'ث', {u'ﺛ', u'ﺜ', u'ﺚ'}}, // Theh
	{u'ج', {u'ﺟ', u'ﺠ', u'ﺞ'}}, // Jeem
	{u'ح', {u'ﺣ', u'ﺤ', u'ﺢ'}}, // Hah
	{u'خ', {u'ﺧ', u'ﺨ', u'ﺦ'}}, // Khah
	{u'د', {u'د', u'ﺪ', u'ﺪ'}}, // Dal
	{u'ذ', {u'ذ', u'ﺬ', u'ﺬ'}}, // Thal
	{u'ر', {u'ر', u'ﺮ', u'ﺮ'}}, // Reh
	{u'ز', {u'ز', u'ﺰ', u'ﺰ'}}, // Zain
	{u'س', {u'ﺳ', u'ﺴ', u'ﺲ'}}, // Seen
	{u'ش', {u'ﺷ', u'ﺸ', u'ﺶ'}}, // Sheen
	{u'ص', {u'ﺻ', u'ﺼ', u'ﺺ'}}, // Sad
	{u'ض', {u'ﺿ', u'ﻀ', u'ﺾ'}}, // Dad
	{u'ط', {u'ﻃ', u'ﻄ', u'ﻂ'}}, // Tah
	{u'ظ', {u'ﻇ', u'ﻈ', u'ﻆ'}}, // Zah
	{u'ع', {u'ﻋ', u'ﻌ', u'ﻊ'}}, // Ain
	{u'غ', {u'ﻏ', u'ﻐ', u'ﻎ'}}, // Ghain
	{u'ػ', {u'ػ', u'ػ', u'ػ'}}, // Keheh with two dots above
	{u'ؼ', {u'ؼ', u'ؼ', u'ؼ'}}, // Keheh with three dots below
	{u'ؽ', {u'ؽ', u'ؽ', u'ؽ'}}, // Farsi yeh with inverted v
	{u'ؾ', {u'ؾ', u'ؾ', u'ؾ'}}, // Farsi yeh with two dots above
	{u'ؿ', {u'ؿ', u'ؿ', u'ؿ'}}, // Farsi yeh with three docs above
	{u'ـ', {u'ـ', u'ـ', u'ـ'}}, // Tatweel
	{u'ف', {u'ﻓ', u'ﻔ', u'ﻒ'}}, // Feh
	{u'ق', {u'ﻗ', u'ﻘ', u'ﻖ'}}, // Qaf
	{u'ك', {u'ﻛ', u'ﻜ', u'ﻚ'}}, // Kaf
	{u'ل', {u'ﻟ', u'ﻠ', u'ﻞ'}}, // Lam
	{u'م', {u'ﻣ', u'ﻤ', u'ﻢ'}}, // Meem
	{u'ن', {u'ﻧ', u'ﻨ', u'ﻦ'}}, // Noon
	{u'ه', {u'ﻫ', u'ﻬ', u'ﻪ'}}, // Heh
	{u'و', {u'و', u'ﻮ', u'ﻮ'}}, // Waw
	{u'ى', {u'ﯨ', u'ﯩ', u'ﻰ'}}, // Alef maksura
	{u'ي', {u'ﻳ', u'ﻴ', u'ﻲ'}}, // Yeh

	{u'ﻻ', {u'ﻻ', u'ﻼ', u'ﻼ'}}, // Ligature lam with alef
};

// Can't do a binary search as the maps aren't fully sorted
constexpr char getIndex(char16_t c) {
	for(uint i = 0; i < 0x80; i++) {
		if(c == mapAscii[i]) {
			return i;
		}
	}

	for(uint i = 0; i < 0x80; i++) {
		if(c == extendedMaps[u8(extendedFont)][i]) {
			return 0x80 + i;
		}
	}

	return '?';
}

// Specifically the Arabic letters that have supported presentation forms
bool isArabic(char16_t c) {
	return (c >= 0x0622 && c <= 0x064A) || c == 0xFEFB;
}

bool isStrongRTL(char16_t c) {
	// Hebrew, Arabic, or RLM
	return (c >= 0x0590 && c <= 0x05FF) || (c >= 0x0600 && c <= 0x06FF) || (c >= 0xFE70 && c <= 0xFEFC) || c == 0x200F;
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
				return arabicPresentationForms[current][1];
			else // If not, final
				return arabicPresentationForms[current][2];
		} else {
			if(isArabic(next)) // If next is arabic, initial
				return arabicPresentationForms[current][0];
			else // If not, isolated
				return current;
		}
	}

	return current;
}

#include <nds.h>
#include <string>

void processRTL(unsigned char *begin, unsigned char *end) {
	unsigned char *ltrBegin = end, *ltrEnd = end;
	bool rtl = true;
	unsigned char buffer[end - begin + 1] = {0};
	unsigned char *res = buffer;

	// Loop through string and save to buffer
	for(unsigned char *p = end - 1; true; p += (rtl ? -1 : 1)) {
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

		char16_t c = *p < 0x80 ? mapAscii[*p] : extendedMaps[u8(extendedFont)][(*p) - 0x80];
		nocashMessage(("a" + std::to_string(c)).c_str());

		// If at the end of an LTR section within RTL, jump back to the RTL
		if(p == ltrEnd && ltrBegin != end) {
			if(ltrBegin == begin && (!isWeak(*ltrBegin) || isNumber(*ltrBegin)))
				break;

			p = ltrBegin;
			c = *p < 0x80 ? mapAscii[*p] : extendedMaps[u8(extendedFont)][(*p) - 0x80];
			ltrBegin = end;
			rtl = true;
		// If in RTL and hit a non-RTL character that's not punctuation, switch to LTR
		} else if(rtl && !isStrongRTL(c) && (!isWeak(c) || isNumber(c))) {
			// Save where we are as the end of the LTR section
			ltrEnd = p + 1;

			// Go back until an RTL character or the start of the string
			bool allNumbers = true;
			while(!isStrongRTL(c) && p != begin) {
				// Check for if the LTR section is only numbers,
				// if so they won't be removed from the end
				if(allNumbers && !isNumber(c) && !isWeak(c))
					allNumbers = false;
				p--;
				c = *p < 0x80 ? mapAscii[*p] : extendedMaps[u8(extendedFont)][(*p) - 0x80];
			}

			// Save where we are to return to after printing the LTR section
			ltrBegin = p;

			// If on an RTL char right now, add one
			if(isStrongRTL(c)) {
				p++;
				c = *p < 0x80 ? mapAscii[*p] : extendedMaps[u8(extendedFont)][(*p) - 0x80];
			}

			// Remove all punctuation and, if the section isn't only numbers,
			// numbers from the end of the LTR section
			if(allNumbers) {
				while(isWeak(c) && !isNumber(c)) {
					if(p != begin)
						ltrBegin++;
					p++;
					c = *p < 0x80 ? mapAscii[*p] : extendedMaps[u8(extendedFont)][(*p) - 0x80];
				}
			} else {
				while(isWeak(c)) {
					if(p != begin)
						ltrBegin++;
					p++;
					c = *p < 0x80 ? mapAscii[*p] : extendedMaps[u8(extendedFont)][(*p) - 0x80];
				}
			}

			// But then allow all numbers directly touching the strong LTR or with 1 weak between
			while((p - 1 >= begin && isNumber(p[-1])) || (p - 2 >= begin && isWeak(p[-1]) && isNumber(p[-2]))) {
				if(p - 1 != begin)
					ltrBegin--;
				p--;
			}
			c = *p < 0x80 ? mapAscii[*p] : extendedMaps[u8(extendedFont)][(*p) - 0x80];

			rtl = false;
		}

		// Brackets are flipped in RTL
		if(rtl) {
			switch(c) {
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
				{
					char16_t prev = p > begin ? (p[-1] < 0x80 ? p[-1] : extendedMaps[u8(extendedFont)][p[-1] - 0x80]) : 0;
					char16_t next = p < end - 1 ? (p[1] < 0x80 ? p[1] : extendedMaps[u8(extendedFont)][p[1] - 0x80]) : 0;
					*(res++) = getIndex(arabicForm(c, prev, next));
					break;
				}
			}
		} else {
			*(res++) = *p;
		}
	}

	tonccpy(begin, buffer, sizeof(buffer));
}

void setIgmString(const char *src, unsigned char *dst) {
	bool containsRTL = false;
	unsigned char *dstStart = dst;

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

		if(codepoint == u'ا' && dst > dstStart && dst[-1] == getIndex(u'ل')) { // لا ligature
			dst[-1] = getIndex(u'ﻻ');
		} else { // Add to string
			*(dst++) = getIndex(codepoint);
		}


		if((codepoint >= 0x0590 && codepoint <= 0x05FF) || (codepoint >= 0x0600 && codepoint <= 0x06FF) || codepoint == 0x200F)
			containsRTL = true;
	}

	// Null terminator
	*dst = 0;

	if(containsRTL)
		processRTL(dstStart, dst);
}
