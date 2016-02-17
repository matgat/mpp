#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector> // Used to collect paths
//#include <map> // Used for extensions map - already in unt_Dictionary.h
//#include <exception>
//---------------------------------------------------------------------------
#include "unt_Dictionary.h" // 'cls_Dictionary'
#include "unt_PoorMansUnicode.h" // 'CheckBOM'
#include "unt_MatUts.h" // 'split_path'


//---------------------------------------------------------------------------
// Invert a map
template<typename K,typename V> void invert_map(std::map<K,V>& m, const bool strict =true)
{
    std::map<K,V> m_inv;
    for(auto i=m.begin(); i!=m.end(); ++i) // for( auto i : m )
       {
        // Handle aliases
        auto has = m_inv.find(i->second);
        if( has != m_inv.end() )
             {// Already existing
              if(strict) throw std::runtime_error("cannot invert map, multiple values for " + has->first);
              //else has->second = i->first; // Take last instead of keeping the first
             }
        else {
              m_inv[i->second] = i->first;
              //auto ins = m_inv.insert( std::map<K,V>::value_type( i->second, i->first ) );
              //if( !ins.second ) throw std::runtime_error("cannot invert map, cannot insert " + i->second);
             }
       }
   // Finally, assign the inverted map
   m = m_inv;
} // 'invert_map'


//---------------------------------------------------------------------------
// Process a file: my parser to tokenize properly
// TODO 1: should manage comments basing on extension
// TODO 3: manage inlined directives: #define, #ifdef, #else, #endif
int Process(const std::string& pth_in, const std::string& pth_out, const cls_Dictionary& dict, const bool overwrite)
{
    // (0) See extension?
    //mat::split_path(pth_in,dir,nam,ext); if( ext==".def" )...
    if( pth_in==pth_out )
       {
        std::cerr << "  Same output file name! " << pth_in << std::endl;
        return 1;
       }

    // (1) Open input file for read
    std::cout << std::endl << '<' << pth_in << '>' << std::endl;
    std::ifstream fin( pth_in, std::ios::binary );
    if( !fin )
       {
        std::cerr << "  Cannot read the file!" << std::endl;
        return 1;
       }

    // (2) Open output file for write
    std::cout << "  > " << pth_out << std::endl;
    // Check overwrite flag
    if( !overwrite )
       {// If must not overwrite, check existance
        std::ifstream fout(pth_out);
        if( fout.good() )
           {
            std::cerr << "  Output file already existing!! (use -f to force overwrite)" << std::endl;
            return 1;
           }
       }
    std::ofstream fout( pth_out, std::ios::binary ); // Overwrite
    if( !fout )
       {
        std::cerr << "  Cannot write the output file!" << std::endl;
        return 1;
       }

    // (3) Parse the file
    enum{ ST_SKIPLINE, ST_SEEK, ST_COLLECT } status = ST_SEEK;
    int n_tok=0, n_sub=0; // Number of encountered tokens and substitutions
    int l = 1; // Current line number
    std::string tok; // Bag for current token
    bool skipsub = false; // Auxiliary to handle '#' for skipping substitution
    unsigned int sqbr_opened = 0; // Auxiliary to handle square brackets in token
    // TODO 5: should deal with encoding, now supporting just 8bit enc
    EN_ENCODING enc = mat::CheckBOM( fin, &fout );
    if( enc != ANSI )
       {
        std::cerr << "  Cannot handle this encoding!" << std::endl;
        return 1;
       }
    // Get the rest
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
                         c=='!' || c=='&' || c=='|' || c=='^' ||
                         c==':' || c==',' || c=='.' ||
                         // c!=';' || c!='/' ||  // <comment chars>
                         c=='\'' || c=='\"' || c=='\\' ) //
                     {// Skip operators
                      fout << c;
                      c = fin.get();
                     }
                else if( c=='/' )
                     {// Skip division/detect c++ comment line
                      fout << c;
                      c = fin.get();
                      if( c=='/' )
                         {
                          fout << c;
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
                else {// Got a token: initialize status to get a new one
                      sqbr_opened = 0;
                      // Handle the 'no-substitution' character
                      if( c=='#' )
                           {
                            skipsub = true;
                            tok = "";
                           }
                      else {
                            skipsub = false;
                            tok = c;
                           }
                      c = fin.get(); // Next
                      status = ST_COLLECT;
                     }
                break;

            case ST_COLLECT : // Collect the rest of the token
                // The token ends with control chars or operators
                while( c!=EOF && c>' ' &&
                       c!='+' && c!='-' && c!='*' && c!='=' &&
                       c!='(' && c!=')' && // c!='[' && c!=']' && (can be part of the token)
                       c!='{' && c!='}' && c!='<' && c!='>' &&
                       c!='!' && c!='&' && c!='|' && c!='^' &&
                       c!=':' && c!=',' && // c!='.' && (can be part of the token)
                       c!=';' && c!='/' && // <comment chars>
                       c!='\'' && c!='\"' && c!='\\' )
                   {
                    if(c=='[') ++sqbr_opened;
                    else if(c==']')
                       {
                        if(sqbr_opened>0) --sqbr_opened;
                        //else // closing a not opened '['!!
                       }
                    tok += c; // potrei solo accettare numeri if(sqbr_opened>0 && !isnumber(c)) break
                    c = fin.get(); // Next
                    if( sqbr_opened==0 && c==']' ) break; // Detect square bracket close
                   }

                // Use token
                ++n_tok;
                // See if it's a defined macro
                auto def = dict.find(tok);
                if( def != dict.end() )
                     {// Got a macro
                      if(skipsub)
                           {// Don't substitute, leave out the '#'
                            fout << tok;
                           }
                      else {// Substitute
                            fout << def->second;
                            ++n_sub;
                           }
                     }
                else {// Not a define, pass as it is
                      if(skipsub) fout << '#'; // Wasn't a define, re-add the '#'
                      fout << tok;
                     }

                // Finally
                status = ST_SEEK;
                break;

           } // 'switch(status)'
       } // 'while(!EOF)'

    // (4) Finally
    //fout.flush(); // Ensure to write the disk
    std::cout << "  Expanded " << n_sub << " macros checking a total of " << n_tok << " tokens in " << l << " lines" << std::endl;
    return 0;
} // 'Process'


//---------------------------------------------------------------------------
enum {RET_OK=0, RET_ARGERR=1, RET_IOERR=2, RET_DEFERR=2, RET_PRCERR=3 };
int main( int argc, const char* argv[] )
{
    // (0) Local objects
    std::ios_base::sync_with_stdio(false); // Try to have better performance
    cls_Dictionary dict;
    struct ST_PAIR { std::string in,out; ST_PAIR(const std::string& s):in{s}{} };
    std::vector<ST_PAIR> files;
    std::map<std::string,std::string> extmap = { {".fst", ".ncs"},
                                                 {".nc", ".ncs"}  };
    // (1) Command line arguments
    std::cout << "mpp (" << __DATE__ << ")\n";
    //std::cout << "Running in: " << argv[0] << std::endl;
    bool including = false, // expected a definition file path
         getting = false,   // expected output file path
         mapping = false;   // expected an extensions mapping string
    bool inv = false;
    bool overwrite = false;
    for( int i=1; i<argc; ++i )
       {
        std::string arg( argv[i] );
        if( arg[0] == '-' )
             {// A command switch
              // Check preesisting state
              if( including )
                 {
                  std::cerr << "!! Expected a path after -i instead of: " << arg << std::endl;
                  return RET_ARGERR;
                 }
              else if( getting )
                 {
                  std::cerr << "!! Expected a path after - instead of: " << arg << std::endl;
                  return RET_ARGERR;
                 }
              else if( mapping )
                 {
                  std::cerr << "!! Expected an extmap after -m instead of: " << arg << std::endl;
                  return RET_ARGERR;
                 }
              // Now see the option
              if( arg.length()==1 )
                    {// Follows a path, next path is the output file
                     if( files.empty() )
                        {
                         std::cerr << "!! No input file specified before " << arg << std::endl;
                         return RET_ARGERR;
                        }
                     getting = true;
                    }
              else if( arg.length()!=2 )
                   {
                    std::cerr << "!! Wrong command switch " << arg << std::endl;
                    return RET_ARGERR;
                   }
              else if( arg[1]=='i' )
                   {// Next path is a definition file to include in the dictionary
                    including = true;
                   }
              else if( arg[1]=='x' )
                   {// Must invert the dictionary
                    inv = true;
                   }
              else if( arg[1]=='m' )
                   {// Mapping extensions
                    mapping = true;
                   }
              else if( arg[1]=='f' )
                   {// Force overwrite
                    overwrite = true;
                   }
              else {
                    std::cerr << "!! Unknown command switch " << arg << std::endl;
                    std::cerr << "   Usage:" << std::endl;
                    std::cerr << "   mpp -m .ncs=.fst -i defvar.def -f *.ncs" << std::endl;
                    std::cerr << "   mpp -x -i defvar.h \"in.ncs\" - \"out.nc\"" << std::endl;
                    return RET_ARGERR;
                   }
             }
        else {// A file path
              if( including )
                   {// Expected a definition file to include
                    including = false; // 'eat'
                    int issues = dict.LoadFile( arg );
                    if( issues>0 )
                       {
                        std::cerr << "! " << issues << " issues including " << arg << std::endl;
                        return RET_DEFERR;
                       }
                   }
              else if( getting )
                   {// Expected an output file path
                    getting = false; // 'eat'
                    files.back().out = arg;
                   }
              else if( mapping )
                   {// Expected something like: .xxx=.yyy
                    mapping = false; // 'eat'
                    std::string::size_type p = arg.find('=');
                    if( p!=std::string::npos && arg[0]=='.' )
                         {
                          std::string ext1 = mat::tolower( arg.substr(0, p) );
                          std::string ext2 = mat::tolower( arg.substr(p+1) );
                          // Check uniqueness in map
                          for( const auto& e : extmap )
                             {
                              if( e.first==ext1 )
                                 {
                                  std::cerr << "!! Invalid extension map " << arg << std::endl;
                                  std::cerr << "   \'" << ext1 << "\' was already mapped" << std::endl;
                                  return RET_ARGERR;
                                 }
                              if( e.second==ext2 )
                                 {
                                  std::cerr << "!! Invalid extension map " << arg << std::endl;
                                  std::cerr << "   \'" << ext2 << "\' was already mapped" << std::endl;
                                  return RET_ARGERR;
                                 }
                             }
                          // If here, insert
                          extmap[ext1] = ext2;
                         }
                    else {
                          std::cerr << "!! Invalid extension map " << arg << std::endl;
                          std::cerr << "   Expected something like: .xxx=.yyy" << std::endl;
                          return RET_ARGERR;
                         }
                   }
              else {
                    files.push_back( ST_PAIR{arg} );
                   }
             }
       }
    if(inv)
       {
        dict.Invert(true); // Exclude numbers
        invert_map(extmap, false); // Don't be strict
       }
    //dict.Peek();

    // (2) Process the files
    int issues = 0;
    for( auto& i : files ) // for( auto i=files.begin(); i!=files.end(); ++i )
       {
        // Corresponding output file name
        if( i.out.empty() )
           {
            std::string dir,nam,ext;
            mat::split_path(i.in, dir,nam,ext);
            auto e = extmap.find(ext);
            if( e!=extmap.end() )
                 {// Got a corresponding entry in mapped extension
                  i.out = dir + nam + e->second;
                 }
            else {// Inventing the transformed file name
                  //i->out = i->in + ".~";
                  i.out = dir + "~" + nam + ext;
                 }
           }
        // Perform operation
        issues += Process(i.in, i.out, dict, overwrite);
       } // All the input files

    // (3) Finally
    if( issues>0 ) // Check issues
       {
        std::cout << std::endl << issues << " issues were found!" << std::endl;
        return RET_PRCERR;
       }
    return RET_OK;
} // end 'main'
