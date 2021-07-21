#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
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
    bool including = false, // expected a definition file path
         getting = false,   // expected output file path
         mapping = false,   // expected an extensions mapping string
         declcmtchar = false;   // expected line comment char
    bool inv = false,
         verbose = false,
         overwrite = false;
    char cmtchar = '\x03';
    for( int i=1; i<argc; ++i )
       {
        std::string_view arg( argv[i] );
        if( arg[0] == '-' )
             {// A command switch
              // Check preesisting state
              if( including )
                 {
                  std::cerr << "!! Expected a path after -i instead of: " << arg << '\n';
                  return RET_ARGERR;
                 }
              else if( getting )
                 {
                  std::cerr << "!! Expected a path after - instead of: " << arg << '\n';
                  return RET_ARGERR;
                 }
              else if( mapping )
                 {
                  std::cerr << "!! Expected an extmap after -m instead of: " << arg << '\n';
                  return RET_ARGERR;
                 }
              else if( declcmtchar )
                 {
                  std::cerr << "!! Expected a line cmt char after -c instead of: " << arg << '\n';
                  return RET_ARGERR;
                 }
              // Now see the option
              if( arg.length()==1 )
                    {// Follows a path, next path is the output file
                     if( files.empty() )
                        {
                         std::cerr << "!! No input file specified before " << arg << '\n';
                         return RET_ARGERR;
                        }
                     getting = true;
                    }
              else if( arg.length()!=2 )
                   {
                    std::cerr << "!! Wrong command switch " << arg << '\n';
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
              else if( arg[1]=='v' )
                   {// Force overwrite
                    verbose = true;
                   }
              else if( arg[1]=='c' )
                   {// Declare line comment char
                    declcmtchar = true;
                   }
              else {
                    std::cerr << "!! Unknown command switch " << arg << '\n';
                    std::cerr << "   Usage:\n";
                    std::cerr << "   mpp -v -m .ncs=.fst -i defvar.def -f *.ncs\n";
                    std::cerr << "   mpp -x -i defvar.h \"in.nc\" - \"out.ncs\"\n";
                    std::cerr << "       -v: verbose\n";
                    std::cerr << "       -f: force overwrite\n";
                    std::cerr << "       -x: invert the dictionary\n";
                    std::cerr << "       -i: include definitions file\n";
                    std::cerr << "       -c: declare line comment char\n";
                    std::cerr << "       -m: map automatic output extension\n";
                    return RET_ARGERR;
                   }
             }
        else {// Not a switch
              if( including )
                   {// Expected a definition file to include
                    including = false; // 'eat'
                    int issues = dict.LoadFile(arg, verbose);
                    if( issues>0 )
                       {
                        std::cerr << "! " << issues << " issues including " << arg << '\n';
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
                          //for( const auto& e : extmap )
                          //   {
                          //    if( e.first==ext1 )
                          //       {
                          //        std::cerr << "!! Invalid extension map " << arg << '\n';
                          //        std::cerr << "   \'" << ext1 << "\' was already mapped\n";
                          //        return RET_ARGERR;
                          //       }
                          //    if( e.second==ext2 )
                          //       {
                          //        std::cerr << "!! Invalid extension map " << arg << '\n';
                          //        std::cerr << "   \'" << ext2 << "\' was already mapped as target\n";
                          //        return RET_ARGERR;
                          //       }
                          //   }
                          // If here, insert/overwrite
                          extmap[ext1] = ext2;
                         }
                    else {
                          std::cerr << "!! Invalid extension map " << arg << '\n';
                          std::cerr << "   Expected something like: .xxx=.yyy\n";
                          return RET_ARGERR;
                         }
                   }
              else if( declcmtchar )
                   {
                    declcmtchar = false; // 'eat'
                    if( arg.length()!=1 )
                       {
                        std::cerr << "!! Invalid comment char " << arg << '\n';
                        return RET_ARGERR;
                       }
                    cmtchar = arg[0];
                   }
              else {
                    files.push_back( ST_PAIR{arg} );
                   }
             }
       } // 'for all cmd args'
    if(inv)
       {
        dict.Invert(true, verbose); // Exclude numbers
        invert_map(extmap, false); // Don't be strict
       }
    //dict.Peek();
    if(verbose) std::cout << "mpp (" << __DATE__ << ")\n";
    //if(verbose) std::cout << "Running in: " << argv[0] << '\n';

    // (2) Process the files
    int issues = 0;
    for( auto& i : files ) // for( auto i=files.begin(); i!=files.end(); ++i )
       {
        // Corresponding output file name
        if( i.out.empty() )
           {
            std::string dir,nam,ext;
            mat::split_path(i.in, dir, nam, ext);
            ext = mat::tolower(ext);
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
        // Could manage comments basing on extension
        //if( ext==".ncs" ) cmtchar = ';';
        issues += cls_Dictionary::Process(i.in, i.out, dict, overwrite, verbose, cmtchar);
        // Could perform other tasks basing on output extension
        //mat::split_path(i.out, dir, nam, ext);
        //if(ext==".fst") Mark the file as compiled, setting the DateLastModified = DateCreated
       } // All the input files

    // (3) Finally
    if( issues>0 ) // Check issues
       {
        std::cout << '\n' << issues << " issues were found!\n";
        return RET_PRCERR;
       }
    return RET_OK;

} // 'main'
