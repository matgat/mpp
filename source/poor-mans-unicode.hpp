#ifndef poor_mans_unicode_hpp
#define poor_mans_unicode_hpp
/*  ---------------------------------------------
    A unit that collects some pityful tentatives
    to deal with unicode encodings
    --------------------------------------------- */
    #include <string_view>
    #include <istream> // 'std::istream'
    #include <ostream> // 'std::ostream'


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace enc
{


/////////////////////////////////////////////////////////////////////////////
// Text files byte order mark          +------------------------------------+
//                                     |  Encoding    |   Bytes     | chars |
//                                     |--------------+-------------+-------|
//                                     | UTF-8        | EF BB BF    | ï»¿   |
//                                     | UTF-16 (BE)  | FE FF       | þÿ    |
//                                     | UTF-16 (LE)  | FF FE       | ÿþ    |
//                                     | UTF-32 (BE)  | 00 00 FE FF | ..þÿ  |
class Bom //                           | UTF-32 (LE)  | FF FE 00 00 | ÿþ..  |
{         //                           +------------------------------------+
 private:
    enum EN_ENCODING { ANSI, UTF8, UTF16_LE, UTF16_BE, UTF32_LE, UTF32_BE };

 public:
    explicit Bom( const std::string_view buf, std::size_t& i ) noexcept { retrieve(buf, i); }

    explicit Bom( std::istream& is )
       {
        std::size_t pos = is.tellg(); // should be zero
        char buf[4];
        is.read(buf,4);
        std::size_t i = 0;
        retrieve(std::string_view(buf, is.gcount()), i);
        is.seekg(pos+i); // Eat possible BOM
       }

    bool is_ansi() const noexcept { return i_enc==ANSI; }
    bool is_utf8() const noexcept { return i_enc==UTF8; }
    bool is_utf16_le() const noexcept { return i_enc==UTF16_LE; }
    bool is_utf16_be() const noexcept { return i_enc==UTF16_BE; }
    bool is_utf32_le() const noexcept { return i_enc==UTF32_LE; }
    bool is_utf32_be() const noexcept { return i_enc==UTF32_BE; }
    //std::size_t size() const noexcept { return i_size; }

    std::string to_str() const noexcept
       {
             if( is_ansi() )     return "ansi";
        else if( is_utf8() )     return "utf-8";
        else if( is_utf16_le() ) return "utf-16-le";
        else if( is_utf16_be() ) return "utf-16-be";
        else if( is_utf32_le() ) return "utf-32-le";
        else if( is_utf32_be() ) return "utf-32-be";
       }

    void write_to( std::ostream& os ) const
       {
             if( is_ansi() ) ;
        //else if( is_utf8() )     os << "\xEF\xBB\xBF"; // Nah
        else if( is_utf16_le() ) os << "\xFF\xFE";
        else if( is_utf16_be() ) os << "\xFE\xFF";
        else if( is_utf32_le() ) os << "\xFF\xFE\x00\x00";
        else if( is_utf32_be() ) os << "\x00\x00\xFE\xFF";
       }

 private:
    EN_ENCODING i_enc = ANSI;
    //std::size_t i_size = 0; // [bytes]

    void retrieve( const std::string_view buf, std::size_t& i ) noexcept
       {//                        +------------------------------------+
        //                        |  Encoding    |   Bytes     | chars |
        //                        |--------------+-------------+-------|
        //                        | UTF-8        | EF BB BF    | ï»¿   |
        //                        | UTF-16 (BE)  | FE FF       | þÿ    |
        //                        | UTF-16 (LE)  | FF FE       | ÿþ    |
        //                        | UTF-32 (BE)  | 00 00 FE FF | ..þÿ  |
        //                        | UTF-32 (LE)  | FF FE 00 00 | ÿþ..  |
        //                        +------------------------------------+
        if( i<buf.size() )
           {
            // ---- Check UTF-8 BOM
            if( buf[i]=='\xEF' )
               {// Could be a UTF-8 BOM
                if( ++i<buf.size() && buf[i]=='\xBB' )
                   {// Seems really a UTF-8 BOM
                    if( ++i<buf.size() && buf[i]=='\xBF' )
                       {// It's definitely a UTF-8 BOM
                        //i_size = 3;
                        i_enc = UTF8;
                       }
                    else
                       {//std::cerr << "Detected part of invalid UTF-8 BOM (EF BB)\n";
                        i-=2;
                        i_enc = UTF8;
                       }
                   }
                else
                   {//std::cerr << "Detected part of invalid UTF-8 BOM (EF)\n";
                    --i;
                    i_enc = UTF8;
                   }
               }

            // ---- Check UTF-16/32 (LE) BOM
            else if( buf[i]=='\xFF' )
               {// Could be a UTF-16/32 (LE) BOM
                if( ++i<buf.size() && buf[i]=='\xFE' )
                   {// Seems really a UTF-16/32 (LE) BOM
                    if( ++i<buf.size() && buf[i]=='\x00' )
                       {// Could be a UTF-32 (LE) BOM
                        if( ++i<buf.size() && buf[i]=='\x00' )
                           {// It's definitely a UTF-32 (LE) BOM
                            //i_size = 4;
                            i_enc = UTF32_LE;
                           }
                        else
                           {// It's a UTF-16 (LE) BOM
                            --i;
                            //i_size = 2;
                            i_enc = UTF16_LE;
                           }
                       }
                    else
                       {// It's definitely a UTF-16 (LE) BOM
                        --i;
                        //i_size = 2;
                        i_enc = UTF16_LE;
                       }
                   }
                else
                   {//std::cerr << "Detected part of invalid UTF-16/32 (LE) BOM (FF)\n";
                    --i;
                    i_enc = ANSI;
                   }
               }

            // ---- Check UTF-16 (BE) BOM
            else if( buf[i]=='\xFE' )
               {// Could be a UTF-16 (BE) BOM
                if( ++i<buf.size() && buf[i]=='\xFF' )
                   {// It's definitely a UTF-16 (BE) BOM
                    //i_size = 2;
                    i_enc = UTF16_BE;
                   }
                else
                   {//std::cerr << "Detected part of invalid UTF-16 (BE) BOM (FE)\n";
                    --i;
                    i_enc = ANSI;
                   }
               }

            // ---- Check UTF-32 (BE) BOM
            else if( buf[i]=='\x00' )
               {// Could be a UTF-32 (BE) BOM
                if( ++i<buf.size() && buf[i]=='\x00' )
                   {// Seems really a UTF-16/32 (BE) BOM
                    if( ++i<buf.size() && buf[i]=='\xFE' )
                       {// It's quite surely a UTF-32 (BE) BOM
                        if( ++i<buf.size() && buf[i]=='\xFF' )
                           {// It's definitely a UTF-32 (BE) BOM
                            //i_size = 4;
                            i_enc = UTF32_BE;
                           }
                        else
                           {//std::cerr << "Detected part of invalid UTF-32 (BE) BOM (00 00 FE)\n";
                            i-=3;
                            i_enc = ANSI;
                           }
                       }
                    else
                       {//std::cerr << "Detected part of invalid UTF-32 (BE) BOM (00 00)\n";
                        i-=2;
                        i_enc = ANSI;
                       }
                   }
                else
                   {//std::cerr << "Detected part of invalid UTF-32 (BE) BOM (00)\n";
                    --i;
                    i_enc = ANSI;
                   }
               }
           }
       }
};



/*
// Character getter
//    enc::ReadChar<true,char16_t> get; // utf-16 le
//    char16_t c; std::istream is;
//    while( get(c,is) ) ...

/////////////////////////////////////////////////////////////////////////////
// Generic implementation (couldn't resist to put one)
template<bool LE,typename T> class ReadChar
{
 public:
    std::istream& operator()(T& c, std::istream& is)
       {
        is.read(buf,bufsiz);
        //const std::streamsize n_read = is ? bufsiz : is.gcount();
        if(!is)
           {// Could not real all bytes
            c = std::char_traits<T>::eof();
           }
        else if constexpr (LE)
           {// Little endian
            c = buf[0];
            for(int i=1; i<bufsiz; ++i) c |= buf[i] << (8*i);
           }
        else
           {// Big endian
            const std::size_t imax = bufsiz-1;
            for(std::size_t i=0; i<imax; ++i) c |= buf[i] << (8*(imax-i));
            c |= buf[imax];
           }
        return is;
       }

 private:
    static constexpr std::size_t bufsiz = sizeof(T);
    unsigned char buf[bufsiz];
};

/////////////////////////////////////////////////////////////////////////////
// Partial specialization for 32bit chars
template<bool LE> class ReadChar<LE,char32_t>
{
 public:
    std::istream& operator()(char32_t& c, std::istream& is)
       {
        is.read(buf,4);
        if constexpr (LE) c = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24); // Little endian
        else              c = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]; // Big endian
        return is;
       }

 private:
    char buf[4];
};

/////////////////////////////////////////////////////////////////////////////
// Partial specialization for 16bit chars
template<bool LE> class ReadChar<LE,char16_t>
{
 public:
    std::istream& operator()(char16_t& c, std::istream& is)
       {
        is.read(buf,2);
        if constexpr (LE) c = buf[0] | (buf[1] << 8); // Little endian
        else              c = (buf[0] << 8) | buf[1]; // Big endian
        return is;
       }

 private:
    char buf[2];
};

/////////////////////////////////////////////////////////////////////////////
// Specialization for 8bit chars
template<> class ReadChar<false,char>
{
 public:
    std::istream& operator()(char& c, std::istream& is)
       {
        return is.get(c);
       }
};
*/



}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//void ToUTF8(char16_t *str) {
//    while (*str) {
//        unsigned int codepoint = 0x0;
//
//        //-------(1) UTF-16 to codepoint -------
//
//        if (*str <= 0xD7FF) {
//            codepoint = *str;
//            str++;
//        } else if (*str <= 0xDBFF) {
//            unsigned short highSurrogate = (*str - 0xD800) * 0x400;
//            unsigned short lowSurrogate = *(str + 1) - 0xDC00;
//            codepoint = (lowSurrogate | highSurrogate) + 0x10000;
//            str += 2;
//        }
//
//        //-------(2) Codepoint to UTF-8 -------
//
//        if (codepoint <= 0x007F) {
//            unsigned char hex[2] = { 0 };
//            hex[0] = (char)codepoint;
//            hex[1] = 0;
//            cout << std::hex << std::uppercase << "(1Byte) " << (unsigned short)hex[0] << endl;
//        } else if (codepoint <= 0x07FF) {
//            unsigned char hex[3] = { 0 };
//            hex[0] = ((codepoint >> 6) & 0x1F) | 0xC0;
//            hex[1] = (codepoint & 0x3F) | 0x80;
//            hex[2] = 0;
//            cout << std::hex << std::uppercase << "(2Bytes) " << (unsigned short)hex[0] << "-" << (unsigned short)hex[1] << endl;
//        } else if (codepoint <= 0xFFFF) {
//            unsigned char hex[4] = { 0 };
//            hex[0] = ((codepoint >> 12) & 0x0F) | 0xE0;
//            hex[1] = ((codepoint >> 6) & 0x3F) | 0x80;
//            hex[2] = ((codepoint) & 0x3F) | 0x80;
//            hex[3] = 0;
//            cout << std::hex << std::uppercase << "(3Bytes) " << (unsigned short)hex[0] << "-" << (unsigned short)hex[1] << "-" << (unsigned short)hex[2] << endl;
//        } else if (codepoint <= 0x10FFFF) {
//            unsigned char hex[5] = { 0 };
//            hex[0] = ((codepoint >> 18) & 0x07) | 0xF0;
//            hex[1] = ((codepoint >> 12) & 0x3F) | 0x80;
//            hex[2] = ((codepoint >> 6) & 0x3F) | 0x80;
//            hex[3] = ((codepoint) & 0x3F) | 0x80;
//            hex[4] = 0;
//            cout << std::hex << std::uppercase << "(4Bytes) " << (unsigned short)hex[0] << "-" << (unsigned short)hex[1] << "-" << (unsigned short)hex[2] << "-" << (unsigned short)hex[3] << endl;
//        }
//    }
//}



/* Windows
//---------------------------------------------------------------------------
// Convert a string encoded in utf-8 to ansi
std::string utf82ansi (const std::string &str, UINT codepage = CP_ACP)
{
    std::string result;
    if (!str.empty())
       {
        int buffLen = ::MultiByteToWideChar( CP_UTF8,0U,str.c_str(),str.size(),NULL,0);
        if (buffLen>0)
           {
            std::vector<wchar_t>buffer(buffLen);
            std::size_t res = ::MultiByteToWideChar( CP_UTF8,0U,str.c_str(),str.size(),&buffer[0],buffer.size());
            if (res>0)
               {
                std::vector<char>sbuffer (buffLen);
                res = ::WideCharToMultiByte( codepage,0U,&buffer[0],buffer.size(),&sbuffer[0],sbuffer.size(),NULL,NULL);
                if (res>0) result.assign(sbuffer.begin(),sbuffer.end());
               }
           }
       }
    return result;
}

int main() {
    std::cout << utf82ansi("<some utf-8 text>") << std::endl;
    return 0;
}



char* _UTF16ToUTF8(  const wchar_t * pszTextUTF16 )
{
    if ( (pszTextUTF16 == NULL) || (*pszTextUTF16 == L'\0') ) return 0;
    int cchUTF16;
    cchUTF16=wcslen( pszTextUTF16)+1;
    int cbUTF8 = WideCharToMultiByte(CP_UTF8,0,pszTextUTF16,cchUTF16,NULL,0,NULL, NULL );
    ASSERT(cbUTF8);
    char *strUTF8=new char[cbUTF8],*pszUTF8 =strUTF8;
    int result = WideCharToMultiByte(CP_UTF8, 0,pszTextUTF16,cchUTF16 ,pszUTF8, cbUTF8,NULL,NULL );
    ASSERT( result);
    return strUTF8;
}


#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
namespace UTF8Util
{
    //----------------------------------------------------------------------------
    // FUNCTION: ConvertUTF8ToUTF16
    // DESC: Converts Unicode UTF-8 text to Unicode UTF-16 (Windows default).
    //----------------------------------------------------------------------------
    CStringW ConvertUTF8ToUTF16( const CHAR * pszTextUTF8 )
    {
        // Special case of NULL or empty input string
        //
        if ( (pszTextUTF8 == NULL) || (*pszTextUTF8 == '\0') ) return L""; // Return empty string

        // Consider CHAR's count corresponding to total input string length,
        // including end-of-string (\0) character
        const size_t cchUTF8Max = INT_MAX - 1;
        size_t cchUTF8;
        HRESULT hr = ::StringCchLengthA( pszTextUTF8, cchUTF8Max, &cchUTF8 );
        if ( FAILED( hr ) )  AtlThrow( hr );

        // Consider also terminating \0
        ++cchUTF8;

        // Convert to 'int' for use with MultiByteToWideChar API
        int cbUTF8 = static_cast<int>( cchUTF8 );

        // Get size of destination UTF-16 buffer, in WCHAR's
        //
        int cchUTF16 = ::MultiByteToWideChar(
        CP_UTF8,                // convert from UTF-8
        MB_ERR_INVALID_CHARS,   // error on invalid chars
        pszTextUTF8,            // source UTF-8 string
        cbUTF8,                 // total length of source UTF-8 string,
        // in CHAR's (= bytes), including end-of-string \0
        NULL,                   // unused - no conversion done in this step
        0                       // request size of destination buffer, in WCHAR's
        );
        ATLASSERT( cchUTF16 != 0 );
        if ( cchUTF16 == 0 ) AtlThrowLastWin32();

        // Allocate destination buffer to store UTF-16 string
        //
        CStringW strUTF16;
        WCHAR * pszUTF16 = strUTF16.GetBuffer( cchUTF16 );

        //
        // Do the conversion from UTF-8 to UTF-16
        //
        int result = ::MultiByteToWideChar(
        CP_UTF8,                // convert from UTF-8
        MB_ERR_INVALID_CHARS,   // error on invalid chars
        pszTextUTF8,            // source UTF-8 string
        cbUTF8,                 // total length of source UTF-8 string,
        // in CHAR's (= bytes), including end-of-string \0
        pszUTF16,               // destination buffer
        cchUTF16                // size of destination buffer, in WCHAR's
        );
        ATLASSERT( result != 0 );
        if ( result == 0 ) AtlThrowLastWin32();

        // Release internal CString buffer
        strUTF16.ReleaseBuffer();

        // Return resulting UTF16 string
        return strUTF16;
    }



    //----------------------------------------------------------------------------
    // FUNCTION: ConvertUTF16ToUTF8
    // DESC: Converts Unicode UTF-16 (Windows default) text to Unicode UTF-8.
    //----------------------------------------------------------------------------
    CStringA ConvertUTF16ToUTF8( const WCHAR * pszTextUTF16 )
    {
        //
        // Special case of NULL or empty input string
        //
        if ( (pszTextUTF16 == NULL) || (*pszTextUTF16 == L'\0') ) return ""; // Return empty string

        // Consider WCHAR's count corresponding to total input string length,
        // including end-of-string (L'\0') character.
        //
        const size_t cchUTF16Max = INT_MAX - 1;
        size_t cchUTF16;
        HRESULT hr = ::StringCchLengthW( pszTextUTF16, cchUTF16Max, &cchUTF16 );
        if ( FAILED( hr ) ) AtlThrow( hr );

        // Consider also terminating \0
        ++cchUTF16;

        // WC_ERR_INVALID_CHARS flag is set to fail if invalid input character
        // is encountered.
        // This flag is supported on Windows Vista and later.
        // Don't use it on Windows XP and previous.
        //
#if (WINVER >= 0x0600)
        DWORD dwConversionFlags = WC_ERR_INVALID_CHARS;
#else
        DWORD dwConversionFlags = 0;
#endif

        // Get size of destination UTF-8 buffer, in CHAR's (= bytes)
        //
        int cbUTF8 = ::WideCharToMultiByte(
        CP_UTF8,                // convert to UTF-8
        dwConversionFlags,      // specify conversion behavior
        pszTextUTF16,           // source UTF-16 string
        static_cast<int>( cchUTF16 ),   // total source string length, in WCHAR's,
        // including end-of-string \0
        NULL,                   // unused - no conversion required in this step
        0,                      // request buffer size
        NULL, NULL              // unused
        );
        ATLASSERT( cbUTF8 != 0 );
        if ( cbUTF8 == 0 ) AtlThrowLastWin32();

        // Allocate destination buffer for UTF-8 string
        //
        CStringA strUTF8;
        int cchUTF8 = cbUTF8; // sizeof(CHAR) = 1 byte
        CHAR * pszUTF8 = strUTF8.GetBuffer( cchUTF8 );

        // Do the conversion from UTF-16 to UTF-8
        //
        int result = ::WideCharToMultiByte(
        CP_UTF8,                // convert to UTF-8
        dwConversionFlags,      // specify conversion behavior
        pszTextUTF16,           // source UTF-16 string
        static_cast<int>( cchUTF16 ),   // total source string length, in WCHAR's,
        // including end-of-string \0
        pszUTF8,                // destination buffer
        cbUTF8,                 // destination buffer size, in bytes
        NULL, NULL              // unused
        );
        ATLASSERT( result != 0 );
        if ( result == 0 ) AtlThrowLastWin32();

        // Release internal CString buffer
        strUTF8.ReleaseBuffer();

        // Return resulting UTF-8 string
        return strUTF8;
    }

} // namespace UTF8Util

*/




/*
#include <string>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <cstdint>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <clocale>
#endif

#define ASSERT_MSG(cond, msg) { if (!(cond)) throw std::runtime_error("Assertion (" #cond ") failed at line " + std::to_string(__LINE__) + "! Msg: " + std::string(msg)); }
#define ASSERT(cond) ASSERT_MSG(cond, "")


template<typename U8StrT = std::string>
inline static U8StrT Utf32To8(const std::u32string& s)
   {
    static_assert(sizeof(typename U8StrT::value_type) == 1, "Char byte-size should be 1 for UTF-8 strings!");
    typedef typename U8StrT::value_type VT;
    typedef uint8_t u8;
    U8StrT r;
    for( auto c : s)
       {
        std::size_t nby = c <= 0x7FU ? 1 : c <= 0x7FFU ? 2 : c <= 0xFFFFU ? 3 : c <= 0x1FFFFFU
                           ? 4
                           : c <= 0x3FFFFFFU ? 5 : c <= 0x7FFFFFFFU ? 6 : 7;
        r.push_back(VT(nby <= 1 ? u8(c) :( (u8(0xFFU) <<( 8 - nby)) | u8(c >>( 6 *( nby - 1))))));
        for( std::size_t i = 1; i < nby; ++i)
            r.push_back(VT(u8(0x80U |( u8(0x3FU) & u8(c >>( 6 *( nby - 1 - i)))))));
       }
    return r;
   }

template <typename U8StrT>
inline static std::u32string Utf8To32(U8StrT const& s)
   {
    static_assert(
        sizeof(typename U8StrT::value_type) == 1, "Char byte-size should be 1 for UTF-8 strings!");
    typedef uint8_t u8;
    std::u32string r;
    auto it =( u8 const*)s.c_str(), end =( u8 const*)(s.c_str() + s.length());
    while( it < end)
       {
        char32_t c = 0;
        if( *it <= 0x7FU)
           {
            c = *it;
            ++it;
           }
        else
           {
            ASSERT((*it & 0xC0U) == 0xC0U);
            std::size_t nby = 0;
            for( u8 b = *it;( b & 0x80U) != 0; b <<= 1, ++nby)
               {
               ( void)0;
               }
            ASSERT(nby <= 7);
            ASSERT((end - it) >= nby);
            c = *it &( u8(0xFFU) >>( nby + 1));
            for( std::size_t i = 1; i < nby; ++i)
               {
                ASSERT((it[i] & 0xC0U) == 0x80U);
                c =( c << 6) |( it[i] & 0x3FU);
               }
            it += nby;
           }
        r.push_back(c);
       }
    return r;
   }


template <typename U16StrT = std::u16string>
inline static U16StrT Utf32To16(std::u32string const& s)
   {
    static_assert(sizeof(typename U16StrT::value_type) == 2,
        "Char byte-size should be 2 for UTF-16 strings!");
    typedef typename U16StrT::value_type VT;
    typedef uint16_t u16;
    U16StrT r;
    for( auto c : s)
       {
        if( c <= 0xFFFFU)
            r.push_back(VT(c));
        else
           {
            ASSERT(c <= 0x10FFFFU);
            c -= 0x10000U;
            r.push_back(VT(u16(0xD800U |( (c >> 10) & 0x3FFU))));
            r.push_back(VT(u16(0xDC00U |( c & 0x3FFU))));
           }
       }
    return r;
   }

template <typename U16StrT>
inline static std::u32string Utf16To32(U16StrT const& s)
   {
    static_assert(sizeof(typename U16StrT::value_type) == 2,
        "Char byte-size should be 2 for UTF-16 strings!");
    typedef uint16_t u16;
    std::u32string r;
    auto it =( u16 const*)s.c_str(), end =( u16 const*)(s.c_str() + s.length());
    while( it < end)
       {
        char32_t c = 0;
        if( *it < 0xD800U || *it > 0xDFFFU)
           {
            c = *it;
            ++it;
           }
        else if( *it >= 0xDC00U)
           {
            ASSERT_MSG(false, "Unallowed UTF-16 sequence!");
           }
        else
           {
            ASSERT(end - it >= 2);
            c =( *it & 0x3FFU) << 10;
            if( (it[1] < 0xDC00U) ||( it[1] > 0xDFFFU))
               {
                ASSERT_MSG(false, "Unallowed UTF-16 sequence!");
               }
            else
               {
                c |= it[1] & 0x3FFU;
                c += 0x10000U;
               }
            it += 2;
           }
        r.push_back(c);
       }
    return r;
   }


template <typename StrT, std::size_t NumBytes = sizeof(typename StrT::value_type)> struct UtfHelper;
template <typename StrT> struct UtfHelper<StrT, 1>
   {
    inline static std::u32string UtfTo32(StrT const & s) { return Utf8To32(s); }
    inline static StrT UtfFrom32(std::u32string const & s) { return Utf32To8<StrT>(s); }
   };
template <typename StrT> struct UtfHelper<StrT, 2>
   {
    inline static std::u32string UtfTo32(StrT const & s) { return Utf16To32(s); }
    inline static StrT UtfFrom32(std::u32string const & s) { return Utf32To16<StrT>(s); }
   };
template <typename StrT> struct UtfHelper<StrT, 4>
{
    inline static std::u32string UtfTo32(StrT const & s)
       {
        return std::u32string((char32_t const *)(s.c_str()), (char32_t const *)(s.c_str() + s.length()));
       }
    inline static StrT UtfFrom32(std::u32string const & s)
       {
        return StrT((typename StrT::value_type const*)(s.c_str()), (typename StrT::value_type const*)(s.c_str() + s.length()));
       }
};
template <typename StrT> inline static std::u32string UtfTo32(StrT const & s) { return UtfHelper<StrT>::UtfTo32(s); }
template <typename StrT> inline static StrT UtfFrom32(std::u32string const & s) { return UtfHelper<StrT>::UtfFrom32(s); }
template <typename StrToT, typename StrFromT> inline static StrToT UtfConv(StrFromT const & s) { return UtfFrom32<StrToT>(UtfTo32(s)); }

//#define Test(cs) \
//    std::cout << Utf32To8(Utf8To32(std::string(cs))) << ", "; \
//    std::cout << Utf32To8(Utf16To32(Utf32To16(Utf8To32(std::string(cs))))) << ", "; \
//    std::cout << Utf32To8(Utf16To32(std::u16string(u##cs))) << ", "; \
//    std::cout << Utf32To8(std::u32string(U##cs)) << ", "; \
//    std::cout << UtfConv<std::string>(UtfConv<std::u16string>(UtfConv<std::u32string>(UtfConv<std::u32string>(UtfConv<std::u16string>(std::string(cs)))))) << ", "; \
//    std::cout << UtfConv<std::string>(UtfConv<std::wstring>(UtfConv<std::string>(UtfConv<std::u32string>(UtfConv<std::u32string>(std::string(cs)))))) << ", "; \
//    std::cout << UtfFrom32<std::string>(UtfTo32(std::string(cs))) << ", "; \
//    std::cout << UtfFrom32<std::string>(UtfTo32(std::u16string(u##cs))) << ", "; \
//    std::cout << UtfFrom32<std::string>(UtfTo32(std::wstring(L##cs))) << ", "; \
//    std::cout << UtfFrom32<std::string>(UtfTo32(std::u32string(U##cs))) << std::endl; \
//    std::cout << "UTF-8 num bytes: " << std::dec << Utf32To8(std::u32string(U##cs)).size() << ", "; \
//    std::cout << "UTF-16 num bytes: " << std::dec << (Utf32To16(std::u32string(U##cs)).size() * 2) << std::endl;
//int main()
//{
//  #ifdef _WIN32
//    SetConsoleOutputCP(65001);
//  #else
//    std::setlocale(LC_ALL, "en_US.UTF-8");
//  #endif
//    try{
//        Test("World");
//        Test("Привет");
//        Test("𐐷𤭢");
//        Test("𝞹");
//        return 0;
//       }
//    catch (std::exception const & ex)
//       {
//        std::cout << "Exception: " << ex.what() << std::endl;
//        return -1;
//       }
//}
*/

//---- end unit -------------------------------------------------------------
#endif
