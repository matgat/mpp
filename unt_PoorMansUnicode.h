#ifndef UNT_POORMANSUNICODE_H
#define UNT_POORMANSUNICODE_H
/*  ---------------------------------------------
    unt_PoorMansUnicode.h
    ©2015 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    A unit that collects some pityful tentatives
    to deal with unicode encodings

    LICENSES
    ---------------------------------------------
    Use and modify freely

    EXAMPLE OF USAGE:
    ---------------------------------------------
    #include "unt_PoorMansUnicode.h" // 'CheckBOM'

    DEPENDENCIES:
    ---------------------------------------------     */
    #include <string>

    // . Types/Constants
    enum EN_ENCODING { ANSI, UTF8, UTF16_LE, UTF16_BE, UTF32_LE, UTF32_BE };

namespace nms_Mat //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

    // . Facilities
    //istream& operator>> (istream& in, char16_t& c) { in >> (short) c; return in; }

    //-----------------------------------------------------------------------
    // Lowercase conversion (TODO: bad with UTF8!)
    inline std::string tolower(std::string s)
       {
        for(size_t i=0, imax=s.length(); i<imax; ++i) s[i] = std::tolower(s[i]);
        return s;
       }

    // . Functions
    EN_ENCODING CheckBOM( std::istream& fin, std::ostream* fout =0, bool verbose =false );
    char32_t GetUTF8( std::istream& in ); // Extract an UTF8 character from byte stream

} // 'nms_Mat' ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace mat = nms_Mat; // a short alias


#endif // 'UNT_POORMANSUNICODE_H'
