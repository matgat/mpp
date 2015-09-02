//---------------------------------------------------------------------------
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
EN_ENCODING CheckBOM( std::istream& fin, std::ostream* fout  )
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



/*
//---------------------------------------------------------------------------
char32_t GetUTF8(std::istream& s)
{
    static_assert( sizeof(char32_t)>=4 );
    char c;
    if( s.get(c) )
       {
        if(c < 0x80)
           {// 1-byte code
            return c;
           }
        else if(c < 0xC0)
           { // invalid!
            return '?';
           }
        else if(c < 0xE0)
           {// 2-byte code
            char32_t c_utf8 = (c & 0x1F) << 6;
            if( s.get(c) ) c_utf8 |= (c & 0x3F); //else truncated!
            return c_utf8;
           }
        else if(c < 0xF0)
           {// 3-byte code
            char32_t c_utf8 = (c & 0x0F) << 12;
            if( s.get(c) ) c_utf8 |= (c & 0x3F) <<  6; //else truncated!
            if( s.get(c) ) c_utf8 |= (c & 0x3F); //else truncated!
            return c_utf8;
           }
        else if(c < 0xF8)
           {// 4-byte code
            char32_t c_utf8 = (c & 0x07) << 18;
            if( s.get(c) ) c_utf8 |= (c & 0x3F) << 12; //else truncated!
            if( s.get(c) ) c_utf8 |= (c & 0x3F) <<  6; //else truncated!
            if( s.get(c) ) c_utf8 |= (c & 0x3F); //else truncated!
            return c_utf8;
           }
       }
    return EOT;
} // 'GetUTF8'
*/


/*
    std::ifstream ifs("input.data");
    ifs.imbue(std::locale(std::locale(), new tick_is_space()));
*/

