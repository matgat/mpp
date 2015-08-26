#ifndef CLS_DICTIONARY_H
#define CLS_DICTIONARY_H
/*  ---------------------------------------------
    unt_Dictionary.h
    ©2015 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    A class to implement an associative array
    for macro and their expansion strings

    LICENSES
    ---------------------------------------------
    Use and modify freely

    EXAMPLE OF USAGE:
    ---------------------------------------------
    #include "unt_Dictionary.h" // 'cls_Dictionary'

    DEPENDENCIES:
    ---------------------------------------------     */
    #include <string>
    #include <map>



/////////////////////////////////////////////////////////////////////////////
// Map of macro and their expansion strings /////////////////////////////////
class cls_Dictionary : public std::map<std::string,std::string>
{                     typedef std::map<std::string,std::string> inherited;
 public:
    cls_Dictionary();
    //~cls_Dictionary();

    int LoadFile(const std::string& pth); // Read a definition file

    void Invert(const bool); // Invert the dictionary (can exclude numbers)
    void Peek(); // Debug utility

    static double ToNum(const std::string&, const bool strict =true); // Number conversion

}; // 'cls_Dictionary'


#endif // 'CLS_DICTIONARY_H'
