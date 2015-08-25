#include <iostream>
#include <fstream>
#include <sstream>
//#include <exception>
#include <string>
#include <vector>
#include "unt_Dictionary.h" // 'cls_Dictionary'

enum {RET_OK=0, RET_ARGERR=1, RET_IOERR=2, RET_DEFERR=2, RET_PRCERR=3 };

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
    std::ifstream ifs("input.data");
    ifs.imbue(std::locale(std::locale(), new tick_is_space()));
*/


//---------------------------------------------------------------------------
// Process a file: my parser to tokenize properly
int Process(std::string& pth, const cls_Dictionary& dict)
{
    // (0) See extension
    //std::string ext( pth.find_last_of(".")!=std::string::npos ? pth.substr(pth.find_last_of(".")+1) : "" );
    //if( ext=="def" )

    // (1) Open input file for read
    std::cout << std::endl << '<' << pth << '>' << std::endl;
    std::ifstream fin( pth );
    if( !fin )
       {
        std::cerr << "  Cannot read the file!" << std::endl;
        return 1;
       }

    // (2) Open output file for write
    std::string pth_out = pth + ".tr";
    std::cout << " >> " << pth_out << std::endl;
    std::ofstream fout( pth_out ); // Overwrite
    if( !fout )
       {
        std::cerr << "  Cannot write the output file!" << std::endl;
        return 1;
       }

    // (3) Parse the file
    enum{ ST_SKIPLINE, ST_SEEK, ST_COLLECT } status = ST_SEEK;
    int n_tok = 0; // Number of encountered tokens
    int n_sub = 0; // Number of substitution
    int l = 1; // Current line number
    std::string tok;
    char c = fin.get();
    while( c != EOF )
       {
        //std::cout << c << " line: " << l << " tok:" << tok << " status: " << status << '\n';
        // According to current status
        switch( status )
           {
            case ST_SKIPLINE : // Skip comment line
                fout << c;
                if( c=='\n' )
                   {
                    ++l;
                    status = ST_SEEK;
                   }
                c = fin.get();
                break;

            case ST_SEEK : // Seek next token
                if( c=='\n' )
                   {// Detect possible line break
                    fout << c;
                    ++l;
                    c=fin.get();
                   }
                else if( c<=' ' )
                   {// Skip control characters
                    fout << c;
                    c = fin.get();
                   }
                else if( c=='+' || c=='-' || c=='*' || c=='=' ||
                         c=='(' || c==')' || c=='[' || c==']' ||
                         c=='{' || c=='}' || c=='<' || c=='>' ||
                         c=='.' || c==',' || c==':' ||
                         c=='!' || c=='&' || c=='|' ||
                         c=='\'' || c=='\"' || c=='\\' )
                   {// Skip operators
                    fout << c;
                    c = fin.get();
                   }
                else if( c=='/' )
                   {// Skip division/detect c++ comment line
                    fout << c;
                    if( fin.get()=='/' )
                       {
                        fout << '/';
                        c = fin.get();
                        status = ST_SKIPLINE;
                       }
                   }
                else if( c==';' )
                   {// Skip semicolon/detect Fagor comment line
                    fout << c;
                    c = fin.get();
                    status = ST_SKIPLINE;
                   }
                else status = ST_COLLECT; // Got a token
                break;

            case ST_COLLECT : // Collect the token
                tok = "";
                // The token ends with control chars or operators
                while( c!=EOF && c>' ' &&
                       c!='+' && c!='-' && c!='*' && c!='=' &&
                       c!='(' && c!=')' && // '[', ']' can be part of the token
                       c!='{' && c!='}' && c!='<' && c!='>' &&
                       c!='.' && c!=',' && c!=':' && c!=';' &&
                       c!='!' && c!='&' && c!='|' &&
                       c!='\'' && c!='\"' && c!='\\' && c!='/' )
                   {
                    tok += c;
                    c = fin.get(); // Next
                   }
                // Use token
                ++n_tok;
                auto def = dict.find(tok);
                if( def != dict.end() )
                     {// Got a define, substitute
                      fout << def->second;
                      ++n_sub;
                     }
                else {// Not a define, pass as it is
                      fout << tok;
                     }
                // Finally
                status = ST_SEEK;
                break;

           } // 'switch(status)'
       } // 'while(!EOF)'

    // (4) Finally
    //fout.flush(); // Ensure to write the disk
    std::cout << "  Substituted " << n_sub << " defines checking a total of " << n_tok << " tokens" << std::endl;
    return 0;
} // 'Process'


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
                    if( issues>0 )
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
    //dict.Peek();
    //std::cout << "definition of " << "GUARD_VALUE"  << ": " << dict["GUARD_VALUE"] << std::endl;
    // TODO: invert dict, output file name

    // (2) Process the input files
    int issues = 0;
    for( auto i=in_files.begin(); i!=in_files.end(); ++i )
       {
        issues += Process(*i, dict);
       } // All the input files
    if( issues>0 )
       {
        std::cerr << std::endl << issues << " issues processing files!" << std::endl;
        return RET_PRCERR;
       }

    // (3) Finally
    return RET_OK;
} // end 'main'
