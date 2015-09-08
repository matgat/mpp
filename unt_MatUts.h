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

    //-----------------------------------------------------------------------
    // Tell if a string is a base 10 integer number
    inline bool is_index(const std::string& s)
       {
        if( s.empty() ) return false;
        for(size_t i=0; i<s.length(); ++i) if( s[i]<'0' || s[i]>'9' ) return false;
        return true;
       }

    //-----------------------------------------------------------------------
    // Decompose path
    void split_path(const std::string& pth, std::string& dir, std::string& nam, std::string& ext);


    /*
    //-----------------------------------------------------------------------
    // Change file extension (include dot)
    inline std::string change_ext(const std::string& pth, const std::string& ext2)
       {
        std::string dir,nam,ext;
        split_path(pth,dir,nam,ext);
        return dir+nam+ext2;
       } */


    //-----------------------------------------------------------------------
    double ToNum(const std::string&, const bool strict =true); // Number conversion

} // 'nms_Mat' ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace mat = nms_Mat; // a short alias

#endif // 'UNT_MATUTS_H'
