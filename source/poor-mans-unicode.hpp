#ifndef poor_mans_unicode_hpp
#define poor_mans_unicode_hpp
/*  ---------------------------------------------
    A unit that collects some pityful tentatives
    to deal with unicode encodings
    --------------------------------------------- */
    #include <string_view>
    #include <istream> // std::istream
    #include <ostream> // std::ostream
    #include <cassert> // assertm


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace enc
{


/////////////////////////////////////////////////////////////////////////////
// Text files byte order mark
class Bom
{
 private:
    enum EN_ENCODING { ANSI, UTF8, UTF16_LE, UTF16_BE, UTF32_LE, UTF32_BE };

 public:
    explicit Bom( const std::string_view buf, std::size_t& i ) noexcept
       {
        retrieve(buf, i);
       }

    explicit Bom( std::istream& is )
       {
        const std::size_t pos = is.tellg(); // should be zero
        assert((pos==0) && "BOM should be checked at start of stream");
        char buf[4];
        is.read(buf,4);
        std::size_t i = 0;
        retrieve(std::string_view(buf, is.gcount()), i);
        is.seekg(pos+i); // Eat possible retrieved BOM
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
        using namespace std::literals; // Use "..."s
             if( is_ansi() )     return "ansi"s;
        else if( is_utf8() )     return "utf-8"s;
        else if( is_utf16_le() ) return "utf-16-le"s;
        else if( is_utf16_be() ) return "utf-16-be"s;
        else if( is_utf32_le() ) return "utf-32-le"s;
        else if( is_utf32_be() ) return "utf-32-be"s;
        return "ansi"s;
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


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




//---- end unit -------------------------------------------------------------
#endif
