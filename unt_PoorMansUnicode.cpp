//#include <exception>
#include <cassert> // 'assert'
#include <iostream> // 'std::cerr'
#include <fstream> // 'std::ifstream'
//---------------------------------------------------------------------------
#include "unt_PoorMansUnicode.h" // 'cls_Dictionary'


//---------------------------------------------------------------------------
// Check possible BOM    |  Encoding    |   Bytes     | Chars |
//                       |--------------|-------------|-------|
//                       | UTF-8        | EF BB BF    | ï»¿   |
//                       | UTF-16 (BE)  | FE FF       | þÿ    |
//                       | UTF-16 (LE)  | FF FE       | ÿþ    |
//                       | UTF-32 (BE)  | 00 00 FE FF | ..þÿ  |
//                       | UTF-32 (LE)  | FF FE 00 00 | ÿþ..  |
EN_ENCODING nms_Mat::CheckBOM( std::istream& fin, std::ostream* fout )
{
    char c;
    // See first byte
    if( fin.get(c) )
       {
        // ---- Check UTF-8 BOM
        if( c=='\xEF' )
           {// Could be a UTF-8 BOM
            if( fin.get(c) && c=='\xBB' )
                 {// Seems really a UTF-8 BOM
                  if( fin.get(c) && c=='\xBF' )
                       {// It's definitely a UTF-8 BOM
                        std::cerr << "  Detected a UTF-8 BOM (EF BB BF)" << std::endl;
                        if(fout) *fout << '\xEF' << '\xBB' << '\xBF'; // pass it
                        return UTF8;
                       }
                  else {
                        std::cerr << "  Detected part of invalid UTF-8 BOM (EF BB)" << std::endl;
                        //if(fin) fin.unget(); // No, I have already in in 'c'
                        if(fout) *fout << '\xEF' << '\xBB'; // pass it
                        fin.unget(); // Put back the last byte read
                        return UTF8;
                       }
                 }
            else {
                  std::cerr << "  Detected part of invalid UTF-8 BOM (EF)" << std::endl;
                  if(fout) *fout << '\xEF'; // pass it
                  fin.unget(); // Put back the last byte read
                  return ANSI;
                 }
           } // UTF-8 BOM
        // ---- Check UTF-16/32 (LE) BOM
        else if( c=='\xFF' )
            {// Could be a UTF-16/32 (LE) BOM
             if( fin.get(c) && c=='\xFE' )
                  {// Seems really a UTF-16/32 (LE) BOM
                   if( fin.get(c) && c!='\x00' )
                        {// It's definitely a UTF-16 (LE) BOM
                         std::cerr << "  Detected a UTF-16 (LE) BOM (FF FE)" << std::endl;
                         if(fout) *fout << '\xFF' << '\xFE'; // pass it
                         fin.unget(); // Put back the last byte read
                         return UTF16_LE;
                        }
                   else {// It's quite surely a UTF-32 (LE) BOM
                         if( fin.get(c) && c=='\x00' )
                              {// It's definitely a UTF-32 (LE) BOM
                               std::cerr << "  Detected a UTF-32 (LE) BOM (FF FE 00 00)" << std::endl;
                               if(fout) *fout << '\xFF' << '\xFE' << '\x00' << '\x00'; // pass it
                               return UTF32_LE;
                              }
                         else {
                               std::cerr << "  Detected part of invalid UTF-32 (LE) BOM (FF FE 00)" << std::endl;
                               if(fout) *fout << '\xFF' << '\xFE' << '\x00'; // pass it
                               //TODO: fin.unget(); // Put back the last byte read
                               return UTF16_LE;
                              }
                        }
                  }
             else {
                   std::cerr << "  Detected part of invalid UTF-16/32 (LE) BOM (FF)" << std::endl;
                   //if(fout) *fout << '\xFF'; // pass it
                   fin.unget(); // Put back the last byte read
                   return ANSI;
                  }
            } // UTF-16/32 (LE) BOM
        // ---- Check UTF-16 (BE) BOM
        else if( c=='\xFE' )
            {// Could be a UTF-16 (BE) BOM
             if( fin.get(c) && c=='\xFF' )
                  {// It's definitely a UTF-16 (BE) BOM
                   std::cerr << "  Detected a UTF-16 (BE) BOM (FE FF)" << std::endl;
                   if(fout) *fout << '\xFE' << '\xFF'; // pass it
                   return UTF16_BE;
                  }
             else {
                   std::cerr << "  Detected part of invalid UTF-16 (BE) BOM (FE)" << std::endl;
                   if(fout) *fout << '\xFE'; // pass it
                   fin.unget(); // Put back the last byte read
                   return ANSI;
                  }
            } // UTF-16 (BE) BOM
        // ---- Check UTF-32 (BE) BOM
        else if( c=='\x00' )
            {// Could be a UTF-32 (BE) BOM
             if( fin.get(c) && c=='\x00' )
                  {// Seems really a UTF-16/32 (BE) BOM
                   if( fin.get(c) && c!='\xFE' )
                        {// It's quite surely a UTF-32 (BE) BOM
                         if( fin.get(c) && c=='\xFF' )
                              {// It's definitely a UTF-32 (BE) BOM
                               std::cerr << "  Detected a UTF-32 (BE) BOM (00 00 FE FF)" << std::endl;
                               if(fout) *fout << '\x00' << '\x00' << '\xFE' << '\xFF'; // pass it
                               return UTF32_BE;
                              }
                         else {
                               std::cerr << "  Detected part of invalid UTF-32 (BE) BOM (00 00 FE)" << std::endl;
                               if(fout) *fout << '\x00' << '\x00' << '\xFE'; // pass it
                               fin.unget(); // Put back the last byte read
                               return ANSI;
                              }
                        }
                   else {
                         std::cerr << "  Detected part of invalid UTF-32 (BE) BOM (00 00)" << std::endl;
                         if(fout) *fout << '\x00' << '\x00'; // pass it
                         fin.unget(); // Put back the last byte read
                         return ANSI;
                        }
                  }
             else {
                   std::cerr << "  Detected part of invalid UTF-32 (BE) BOM (00)" << std::endl;
                   if(fout) *fout << '\x00'; // pass it
                   fin.unget(); // Put back the last byte read
                   return ANSI;
                  }
            } // UTF-32 (BE) BOM
       } // Checking first byte
    // If here, no BOM detected
    fin.unget(); // Put back the last byte read
    return ANSI;
} // 'CheckBOM'




//---------------------------------------------------------------------------
// Extract an UTF8 character from byte stream
char32_t nms_Mat::GetUTF8( std::istream& in )
{
    //static_assert( sizeof(char32_t)>=4, "" ); // #include <type_traits>?
    unsigned char c;
    if( in>>c )
       {
        if( c<0x80 )
           {// 1-byte code
            return c;
           }
        else if( c<0xC0 )
           { // invalid!
            return U'?';
           }
        else if( c<0xE0 )
           {// 2-byte code
            char32_t c_utf8 = (c & 0x1F) << 6;
            if( in>>c ) c_utf8 |= (c & 0x3F); //else truncated!
            return c_utf8;
           }
        else if( c<0xF0 )
           {// 3-byte code
            char32_t c_utf8 = (c & 0x0F) << 12;
            if( in>>c ) c_utf8 |= (c & 0x3F) <<  6; //else truncated!
            if( in>>c ) c_utf8 |= (c & 0x3F); //else truncated!
            return c_utf8;
           }
        else if( c<0xF8 )
           {// 4-byte code
            char32_t c_utf8 = (c & 0x07) << 18;
            if( in>>c ) c_utf8 |= (c & 0x3F) << 12; //else truncated!
            if( in>>c ) c_utf8 |= (c & 0x3F) <<  6; //else truncated!
            if( in>>c ) c_utf8 |= (c & 0x3F); //else truncated!
            return c_utf8;
           }
       }
    return EOF;
} // 'GetUTF8'



/*
    std::ifstream ifs("input.data");
    ifs.imbue(std::locale(std::locale(), new tick_is_space()));
*/



/*

#include <iostream>
#include <string>
#include <locale>
std::string Convert(std::string& str)
{
    std::locale settings;
    std::string converted;

    for(short i = 0; i < str.size(); ++i)
        converted += (std::toupper(str[i], settings));

    return converted;
}
int main()
{
    std::string parameter = "lowercase";
    std::cout << Convert(parameter);
    std::cin.ignore();
    return 0;
}


// tolower example (C++)
#include <iostream>       // std::cout
#include <string>         // std::string
#include <locale>         // std::locale, std::tolower
int main ()
{
  std::locale loc;
  std::string str="Test String.\n";
  for (std::string::size_type i=0; i<str.length(); ++i)
    std::cout << std::tolower(str[i],loc);
  return 0;
}


#include <iostream>
#include <clocale>
#include <cwctype>
#include <cstdlib>
int main()
{
    std::setlocale(LC_ALL, "en_US.utf8");

    char utf8[] = {'\xc3', '\x81'};
    wchar_t big;
    std::mbtowc(&big, utf8, sizeof utf8);
    // or just skip the whole utf8 conversion
    //    wchar_t big = L'Á';
    wchar_t small = std::towlower(big);
    std::wcout << "Big: " << big  << '\n'
               << "Small: " << small << '\n';
}

*/
