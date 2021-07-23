/*  ---------------------------------------------
    mpp
    ©2015-2021 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    Simple macro expansion tool

    DEPENDENCIES:
    --------------------------------------------- */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector> // Used to collect paths
#include <map> // Used for extensions map
#include <stdexcept> // 'std::runtime_error'
#include "system.hpp" // 'sys::', 'fs::'
#include "string-utilities.hpp" // 'str::tolower'
#include "logging.hpp" // 'dlg::error'
#include "dictionary.hpp" // 'dict_t'

using namespace std::literals; // Use "..."sv


/////////////////////////////////////////////////////////////////////////////
class Arguments
{
 public:
    Arguments(int argc, const char* argv[])
       {
        //std::vector<std::string> args(argv+1, argv+argc); for( std::string& arg : args )
        // Expecting pll file paths
        i_files.reserve( static_cast<std::size_t>(argc));
        try{
            enum class STS
               {
                SEE_ARG,
                GET_VER,
                GET_OUT
               } status = STS::SEE_ARG;

            for( int i=1; i<argc; ++i )
               {
                const std::string_view arg{ argv[i] };
                switch( status )
                   {
                    case STS::SEE_ARG :
                        if( arg[0] == '-' )
                           {// A command switch!
                            if( arg=="--force" || arg=="-f" )
                               {
                                i_force = true;
                               }
                            else if( arg=="--verbose" || arg=="-v" )
                               {
                                i_verbose = true;
                               }
                            else if( arg=="-sort" )
                               {
                                i_sort = true;
                               }
                            else if( arg=="-schemaver" )
                               {
                                status = STS::GET_VER; // Value expected
                               }
                            else if( arg=="-output" )
                               {
                                status = STS::GET_OUT; // Value expected
                               }
                            else
                               {
                                throw std::invalid_argument("Unknown argument: " + std::string(arg));
                               }
                           }
                        else
                           {// An input file
                            //i_files.emplace_back(arg);
                            const auto globbed = sys::glob(arg);
                            i_files.reserve(i_files.size() + globbed.size());
                            i_files.insert(i_files.end(), globbed.begin(), globbed.end());
                           }
                        break;

                    case STS::GET_VER :
                        i_schemaver = arg; // Expecting something like "2.8"
                        status = STS::SEE_ARG;
                        break;

                    case STS::GET_OUT :
                        i_output = arg; // Expecting a path
                        if( !fs::exists(i_output) ) throw std::invalid_argument("Output path doesn't exists: " + i_output.string());
                        if( !fs::is_directory(i_output) ) throw std::invalid_argument("Combine to same output file not yet supported: " + i_output.string());
                        status = STS::SEE_ARG;
                        break;
                   }
               } // each argument
           }
        catch( std::exception& e)
           {
            throw std::invalid_argument(e.what());
           }
       }

    static void print_usage() noexcept
       {
        std::cerr << "   Usage:\n";
        std::cerr << "   mpp -verbose -force -map .ncs=.fst -map .ncs=.nc -include defs.def *.ncs\n";
        std::cerr << "   mpp -x -include defvar.h \"in.nc\" -out \"out.ncs\"\n";
        std::cerr << "       -verbose: Print more info\n";
        std::cerr << "       -force: Overwrite if existing\n";
        std::cerr << "       -x: invert the dictionary\n";
        std::cerr << "       -include: include definitions file\n";
        std::cerr << "       -cmt: declare line comment char\n";
        std::cerr << "       -map: map automatic output extension\n";
        std::cerr << "       -out: output file\n";
       }

    const auto& files() const noexcept { return i_files; }
    const auto& output() const noexcept { return i_output; }
    bool fussy() const noexcept { return i_fussy; }
    bool sort() const noexcept { return i_sort; }
    bool verbose() const noexcept { return i_verbose; }
    plclib::Version schema_ver() const noexcept { return i_schemaver; }

 private:
    std::vector<fs::path> i_defines;
    std::vector<fs::path> i_files;
    fs::path i_output = ".";
    bool i_verbose = false;
};


/*
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
            else
               {
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
        else
           {// Not a switch
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
                    std::string ext1 = enc::tolower( arg.substr(0, p) );
                    std::string ext2 = enc::tolower( arg.substr(p+1) );
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
                else
                   {
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
            else
               {
                files.push_back( ST_PAIR{arg} );
               }
          }
       } // 'for all cmd args'
*/



//---------------------------------------------------------------------------
int main( int argc, const char* argv[] )
{
    std::ios_base::sync_with_stdio(false); // Better performance

    try{
        Arguments args(argc, argv);

        if (args.verbose())
           {
            std::cout << "**** pll2plclib (" << __DATE__ << ") ****\n"; // sys::human_readable_time_stamp()
            std::cout << "Running in: " << fs::current_path().string() << '\n';
           }
        if( args.files().empty() ) throw std::invalid_argument("No files passed");

        std::size_t n_issues = 0;
        for( const auto& in_file_path : args.files() )

    dict_t dict;
    if(inv)
       {
        dict.Invert(true, verbose); // Exclude numbers
        invert_map(extmap, false); // Don't be strict
       }

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
                ext = enc::tolower(ext);
                auto e = extmap.find(ext);
                if( e!=extmap.end() )
                   {// Got a corresponding entry in mapped extension
                    i.out = dir + nam + e->second;
                   }
                else
                   {// Inventing the transformed file name
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

        if( n_issues>0 )
           {
            std::cerr << "[!] " << n_issues << " issues found\n";
            return -1;
           }
        return 0;
       }
    catch(std::invalid_argument& e)
       {
        std::cerr << "!! " << e.what() << '\n';
        Arguments::print_usage();
       }
    catch(std::exception& e)
       {
        std::cerr << "!! Error: " << e.what() << '\n';
       }
    return -1;
}
