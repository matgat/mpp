#include <iostream>
#include <fstream>
#include <sstream>
//#include <exception>
#include <string>
#include <vector>
#include "unt_Dictionary.h" // 'cls_Dictionary'

enum {RET_OK=0, RET_ARGERR=1, RET_IOERR=2, RET_DEFERR=2 };

//#include <boost/filesystem.hpp>

/*
wchar_t GetUTF8(const std::istream& s)
{
  char c = 0;
  wchar_t ret = '?';
  s >> c;

  if(c < 0x80)      // 1-byte code
    ret = c;
  else if(c < 0xC0)     // invalid
    ;
  else if(c < 0xE0) // 2-byte code
  {
    ret =  (c & 0x1F) << 6;    s >> c;
    ret |= (c & 0x3F);
  }
  else if(c < 0xF0)     // 3-byte code
  {
    ret =  (c & 0x0F) << 12;   s >> c;
    ret |= (c & 0x3F) <<  6;   s >> c;
    ret |= (c & 0x3F);
  }
  else if(c < 0xF8)     // 4-byte code
  {
    // make sure wchar_t is large enough to hold it
    if(std::numeric_limits<wchar_t>::max() > 0xFFFF)
    {
      ret =  (c & 0x07) << 18;   s >> c;
      ret |= (c & 0x3F) << 12;   s >> c;
      ret |= (c & 0x3F) <<  6;   s >> c;
      ret |= (c & 0x3F);
    }
  }

  return ret;
}
*/


/*


#include <iostream>
#include <fstream>
#include <locale>
#include <vector>
#include <sstream>
#include <iomanip>
struct tick_is_space : std::ctype<char> {
    tick_is_space() : std::ctype<char>(get_table()) {}
    static std::ctype_base::mask const* get_table()
    {
        static std::vector<std::ctype_base::mask>
               rc(table_size, std::ctype_base::mask());
        rc['\n'] = std::ctype_base::space;
        rc['\''] = std::ctype_base::space;
        return &rc[0];
    }
};

int main()
{
    std::ifstream ifs("input.data");
    ifs.imbue(std::locale(std::locale(), new tick_is_space()));
    int foo;
    std::string type, ullstr;
    while( ifs >> foo >> type >> ullstr)
    {
        std::vector<unsigned long long> ull;
        while(ullstr.size() >= 16) // sizeof(unsigned long long)*2
        {
            std::istringstream is(ullstr.substr(0, 16));
            unsigned long long tmp;
            is >> std::hex >> tmp;
            ull.push_back(tmp);
            ullstr.erase(0, 16);
        }
        std::cout << std::dec << foo << " " << type << " "
                  << std::hex << std::showbase;
        for(size_t p=0; p<ull.size(); ++p)
            std::cout << std::setw(16) << std::setfill('0') << ull[p] << ' ';
        std::cout << '\n';
    }
}

const Token&
Token& const

/////////////////////////////////////////////////////////////////////////////
///////////////////////////////// cls_Token /////////////////////////////////
class cls_Token : public cls_Token
{
 public:
    //cls_Token(int n) : cls_(n) {;}
    //~cls_Token();
    friend std::ostream& operator<<(std::ostream&,const Token&);
    friend std::istream& operator>>(std::istream&,Token&);

 private:
    std::string value;

}; // end 'cls_Token'

//---------------------------------------------------------------------------
std::istream& operator>>(std::istream& str, cls_Token& data)
{
    if(!str) return str; // Check to make sure the stream is OK

    // Drop leading spaces
    char x;
    do{ x = str.get(); } while(str && isspace(x) && (x!='\n'));
    if(!str) return str; // If the stream is done exit

    data.value = "";
    if(x == '\n')
         {// If the token is a '\n' we are finished
          data.value = "\n";
         }
    else {// Otherwise read the next token in
          str.unget();
          str >> data.value;
         }
    return str;
}

//---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& str,const cls_Token& data)
{
    return str << data.value;
}


int main()
{
    std::ifstream   f("PLOP");
    Token   x;

    while(f >> x)
    {
        std::cout << "Token(" << x << ")\n";
    }
}

        //std::stringstream lineStream(line);
        //std::string token;
        //while(lineStream >> token) std::cout << "Token: " << token << std::endl;
*/



//---------------------------------------------------------------------------
std::string process(const std::string& line)
{
    //std::cout << "line read: " << line << std::endl;
    std::stringstream lineStream(line);
    std::string token;
    while(lineStream >> token)
       {
        std::cout << "Token: " << token << std::endl;
       }
    return line + " <<<<<<";
}

//---------------------------------------------------------------------------
int main( int argc, const char* argv[] )
{
    // (0) Local objects
    cls_Dictionary dict;
    std::vector<std::string> in_files;

    // (1) Command line arguments
    std::cout << "Running in: " << argv[0] << std::endl;
    bool including = false;
    for( int i=1; i<argc; ++i )
       {
        //std::cout << i << " " << argv[i] << std::endl;
        std::string arg( argv[i] );

        if( arg[0] == '-' )
             {// A command switch
              if( arg.length()!=2 )
                    {
                     std::cerr << "!! Wrong command switch " << arg << std::endl;
                     return RET_ARGERR;
                    }
              else if( arg[1]=='i' )
                   {
                    including = true;
                   }
              else {
                    std::cerr << "!! Unknown command switch " << arg << std::endl;
                    return RET_ARGERR;
                   }
             }
        else {// A file path
              if( including )
                   {
                    int issues = dict.LoadFile( arg );
                    including = false;
                    if(issues>0)
                       {
                        std::cerr << "! " << issues << " issues including " << arg << std::endl;
                        return RET_DEFERR;
                       }
                   }
              else {
                    in_files.push_back( arg );
                   }
             }
       }
    std::cout << "definition of " << "GUARD_VALUE"  << ": " << dict["GUARD_VALUE"] << std::endl;

    // (2) Input files
    for( auto i=in_files.begin(); i!=in_files.end(); ++i )
       {
        std::ifstream fin( *i );
        if( !fin )
           {
            std::cerr << "!! Open file for input failed!" << std::endl;
            return RET_IOERR;
           }

        std::ofstream fout( *i + ".transf" );
        if( !fout )
           {
            std::cerr << "!! Open file for output failed!" << std::endl;
            return RET_IOERR;
           }

        //while( fin.good() )
        //   {
        //    char c = fin.get();
        //    std::cout << "got: " << c << std::endl;
        //   }

        std::string line;
        while( std::getline(fin, line) )
           {
            fout << process(line) << std::endl;
           }

        fin.close();
        fout.close();
       }

    // (3) Finally
    return RET_OK;
} // end 'main'
