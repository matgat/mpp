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

// . Types/Constants
    enum EN_ENCODING { ANSI, UTF8, UTF16, UTF32 };

// . Facilities
    //istream& operator>> (istream& in, char16_t& c) { in >> (short) c; return in; }

// . Functions
    EN_ENCODING CheckBOM(char& c, std::istream& fin, std::ostream* fout =0 );


#endif // 'UNT_POORMANSUNICODE_H'
