#ifndef poor_mans_unicode_hpp
#define poor_mans_unicode_hpp
/*  ---------------------------------------------
    A unit that collects some pityful tentatives
    to deal with unicode encodings
    --------------------------------------------- */
    #include <string>
    #include <cctype> // 'tolower'
    //#include <cassert> // 'assert'
    #include <iostream> // 'std::cerr'


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace enc
{

enum EN_ENCODING { ANSI, UTF8, UTF16_LE, UTF16_BE, UTF32_LE, UTF32_BE };


//-----------------------------------------------------------------------
// Lowercase conversion (TODO: bad with UTF8!)
inline std::string tolower(std::string s)
   {
    for(size_t i=0, imax=s.length(); i<imax; ++i) s[i] = std::tolower(s[i]);
    return s;
   }


//---------------------------------------------------------------------------
// Check possible BOM    |  Encoding    |   Bytes     | Chars |
//                       |--------------|-------------|-------|
//                       | UTF-8        | EF BB BF    | ï»¿   |
//                       | UTF-16 (BE)  | FE FF       | þÿ    |
//                       | UTF-16 (LE)  | FF FE       | ÿþ    |
//                       | UTF-32 (BE)  | 00 00 FE FF | ..þÿ  |
//                       | UTF-32 (LE)  | FF FE 00 00 | ÿþ..  |
EN_ENCODING check_bom( std::istream& fin, std::ostream* fout, bool verbose )
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
                        if(verbose) std::cerr << "  Detected a UTF-8 BOM (EF BB BF)\n";
                        if(fout) *fout << '\xEF' << '\xBB' << '\xBF'; // pass it
                        return UTF8;
                       }
                  else {
                        std::cerr << "  Detected part of invalid UTF-8 BOM (EF BB)\n";
                        //if(fin) fin.unget(); // No, I have already in in 'c'
                        if(fout) *fout << '\xEF' << '\xBB'; // pass it
                        fin.unget(); // Put back the last byte read
                        return UTF8;
                       }
                 }
            else {
                  std::cerr << "  Detected part of invalid UTF-8 BOM (EF)\n";
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
                         if(verbose) std::cerr << "  Detected a UTF-16 (LE) BOM (FF FE)\n";
                         if(fout) *fout << '\xFF' << '\xFE'; // pass it
                         fin.unget(); // Put back the last byte read
                         return UTF16_LE;
                        }
                   else {// It's quite surely a UTF-32 (LE) BOM
                         if( fin.get(c) && c=='\x00' )
                              {// It's definitely a UTF-32 (LE) BOM
                               if(verbose) std::cerr << "  Detected a UTF-32 (LE) BOM (FF FE 00 00)\n";
                               if(fout) *fout << '\xFF' << '\xFE' << '\x00' << '\x00'; // pass it
                               return UTF32_LE;
                              }
                         else {
                               std::cerr << "  Detected part of invalid UTF-32 (LE) BOM (FF FE 00)\n";
                               if(fout) *fout << '\xFF' << '\xFE' << '\x00'; // pass it
                               //fin.unget(); // Put back the last byte read
                               return UTF16_LE;
                              }
                        }
                  }
             else {
                   std::cerr << "  Detected part of invalid UTF-16/32 (LE) BOM (FF)\n";
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
                   if(verbose) std::cerr << "  Detected a UTF-16 (BE) BOM (FE FF)\n";
                   if(fout) *fout << '\xFE' << '\xFF'; // pass it
                   return UTF16_BE;
                  }
             else {
                   std::cerr << "  Detected part of invalid UTF-16 (BE) BOM (FE)\n";
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
                               if(verbose) std::cerr << "  Detected a UTF-32 (BE) BOM (00 00 FE FF)\n";
                               if(fout) *fout << '\x00' << '\x00' << '\xFE' << '\xFF'; // pass it
                               return UTF32_BE;
                              }
                         else {
                               std::cerr << "  Detected part of invalid UTF-32 (BE) BOM (00 00 FE)\n";
                               if(fout) *fout << '\x00' << '\x00' << '\xFE'; // pass it
                               fin.unget(); // Put back the last byte read
                               return ANSI;
                              }
                        }
                   else {
                         std::cerr << "  Detected part of invalid UTF-32 (BE) BOM (00 00)\n";
                         if(fout) *fout << '\x00' << '\x00'; // pass it
                         fin.unget(); // Put back the last byte read
                         return ANSI;
                        }
                  }
             else {
                   std::cerr << "  Detected part of invalid UTF-32 (BE) BOM (00)\n";
                   if(fout) *fout << '\x00'; // pass it
                   fin.unget(); // Put back the last byte read
                   return ANSI;
                  }
            } // UTF-32 (BE) BOM
       } // Checking first byte
    // If here, no BOM detected
    fin.unget(); // Put back the last byte read
    return ANSI;
}



//---------------------------------------------------------------------------
// Extract an UTF8 character from byte stream
char32_t get_utf8( std::istream& in )
{
    //static_assert( sizeof(char32_t)>=4, "" ); // #include <type_traits>?
    unsigned char c;
    if( in >> c )
       {
        if( c<0x80 )
           {// 1-byte code
            return c;
           }
        else if( c<0xC0 )
           {// invalid!
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
    return std::char_traits<char32_t>::eof();
}



/////////////////////////////////////////////////////////////////////////////
// A function object that read a char from byte stream
template<typename T,bool E=true> class ReadChar
{
 public:
    std::istream& operator()(T& c, std::istream& in) const
       {
        in.read(buf,bufsiz);
        //const std::streamsize n_read = in ? bufsiz : in.gcount();
        if(!in)
           {// Could not real all bytes
            c = std::char_traits<T>::eof();
           }
        else if(E)
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
        return in;
       }

 private:
    constexpr std::size_t bufsiz = sizeof(T);
    mutable unsigned char buf[bufsiz];
};

/////////////////////////////////////////////////////////////////////////////
// Specialization for 32bit chars little endian
template<> class ReadChar<char32_t,true>
{
 public:
    std::istream& operator()(char32_t& c, std::istream& in)
       {
        in.read(buf,4);
        c = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24); // little endian
        return in;
       }

 private:
    mutable char buf[4];
};

/////////////////////////////////////////////////////////////////////////////
// Specialization for 32bit chars big endian
template<> class ReadChar<char32_t,false>
{
 public:
    std::istream& operator()(char32_t& c, std::istream& in)
       {
        in.read(buf,4);
        c = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]; // big endian
        return in;
       }

 private:
    mutable char buf[4];
};

/////////////////////////////////////////////////////////////////////////////
// Specialization for 16bit chars little endian
template<> class ReadChar<char16_t,true>
{
 public:
    std::istream& operator()(char16_t& c, std::istream& in)
       {
        in.read(buf,2);
        c = buf[0] | (buf[1] << 8); // little endian
        return in;
       }

 private:
    mutable char buf[2];
};

/////////////////////////////////////////////////////////////////////////////
// Specialization for 16bit chars big endian
template<> class ReadChar<char16_t,false>
{
 public:
    std::istream& operator()(char16_t& c, std::istream& in)
       {
        in.read(buf,2);
        c = (buf[0] << 8) | buf[1]; // big endian
        return in;
       }

 private:
    mutable char buf[2];
};

/////////////////////////////////////////////////////////////////////////////
// Specialization for 8bit chars
template<> class ReadChar<char>
{
 public:
    std::istream& operator()(char& c, std::istream& in)
       {
        return in.get(c);
       }
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
