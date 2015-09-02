#ifndef UNT_MATUTS_H
#define UNT_MATUTS_H
/*  ---------------------------------------------
    unt_MatUts.h
    ©2015 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    A unit that collects some utilities

    LICENSES
    ---------------------------------------------
    Use and modify freely

    EXAMPLE OF USAGE:
    ---------------------------------------------
    #include "unt_MatUts.h" // 'mat::ToNum'

    DEPENDENCIES:
    ---------------------------------------------     */
    #include <string>


namespace nms_Mat //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{
 
    double ToNum(const std::string&, const bool strict =true); // Number conversion
    
    //-----------------------------------------------------------------------
    // Lowercase conversion (fails with UTF8!)
    inline std::string& strtolower(std::string& s)
       {
        for(size_t i=0, imax=s.length(); i<imax; ++i) s[i] = tolower(s[i]);
        return s;
       }

    //-----------------------------------------------------------------------
    // Tell if a string is a base 10 integer number
    inline bool is_index(const std::string& s)
       {
        if( s.empty() ) return false;
        for(size_t i=0; i<s.length(); ++i) if( s[i]<'0' || s[i]>'9' ) return false;
        return true;
       }

} // 'nms_Mat' ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace mat = nms_Mat; // a short alias

#endif // 'UNT_MATUTS_H'
