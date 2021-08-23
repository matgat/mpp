/*  ---------------------------------------------
    mpp
    ©2015-2021 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    Simple macro expansion tool

    DEPENDENCIES:
    --------------------------------------------- */
#include <iostream>
#include <string>
#include <string_view>
#include "string-utilities.hpp" // 'str::tolower'
#include <vector> // Collected paths
#include <map> // File extensions mapping
#include <stdexcept> // 'std::runtime_error'
#include "system.hpp" // 'sys::', 'fs::'
#include "logging.hpp" // 'dlg::error'
#include "dictionary.hpp" // 'Dictionary'
#include "preprocess.hpp" // 'pre::process()'

using namespace std::literals; // Use "..."sv


/////////////////////////////////////////////////////////////////////////////
class Arguments
{
 public:
    Arguments(int argc, const char* argv[])
       {
        //std::vector<std::string> args(argv+1, argv+argc); for( std::string& arg : args )
        try{
            enum class STS
               {
                SEE_ARG,
                GET_INC, // expecting a definition file path
                GET_OUT, // expecting a paired output file path
                GET_MAP  // expecting an extension mapping
               } status = STS::SEE_ARG;

            for( int i=1; i<argc; ++i )
               {
                const std::string_view arg{ argv[i] };
                switch( status )
                   {
                    case STS::SEE_ARG :
                        if( arg[0] == '-' )
                           {// A command switch!
                            if( arg=="-f" || arg=="--force" )
                               {
                                i_overwrite = true;
                               }
                            else if( arg=="-v" || arg=="--verbose" )
                               {
                                i_verbose = true;
                               }
                            else if( arg=="-x" ||arg=="-invert" )
                               {
                                i_invert_dict = true;
                               }
                            else if( arg=="-i" || arg=="-include" )
                               {
                                status = STS::GET_INC;
                               }
                            else if( arg=="-o" || arg=="-output" )
                               {
                                status = STS::GET_OUT;
                               }
                            else if( arg=="-m" ||arg=="-map" )
                               {
                                status = STS::GET_MAP;
                               }
                            else
                               {
                                throw std::invalid_argument("Unknown argument: " + std::string(arg));
                               }
                           }
                        else
                           {// An input file
                            auto globbed = sys::glob(arg);
                            i_paths_pairs.reserve(i_paths_pairs.size() + globbed.size());
                            for(auto& pth : globbed)
                               {
                                //i_paths_pairs.emplace_back(pth);
                                i_paths_pairs.emplace_back();
                                i_paths_pairs.back().in = pth;
                               }
                           }
                        break;

                    case STS::GET_INC :
                       {status = STS::SEE_ARG;
                        const auto globbed = sys::glob(arg);
                        i_defines.reserve(i_defines.size() + globbed.size());
                        i_defines.insert(i_defines.end(), globbed.begin(), globbed.end());
                       } break;

                    case STS::GET_OUT :
                        //if( fs::exists(i_output) ) throw std::invalid_argument("Output path already existing: " + i_output.string());
                        if( i_paths_pairs.empty() ) throw std::invalid_argument("Input file not specified for output " + std::string(arg));
                        i_paths_pairs.back().out = arg;
                        break;

                    case STS::GET_MAP :
                        status = STS::SEE_ARG;
                        if( std::string::size_type p = arg.find('=');
                            p!=std::string::npos && arg[0]=='.' )
                           {
                            std::string ext1 = str::tolower( std::string(arg.substr(0,p)) );
                            std::string ext2 = str::tolower( std::string(arg.substr(p+1)) );
                            i_outexts_map[ext1] = ext2;
                           }
                        else
                           {
                            throw dlg::error("Invalid extension map {} (expecting .xxx=.yyy)",arg);
                           }
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
        std::cerr << "   mpp [flags] -i <def-file1> -i <def-file2> <in-file1> -o <out-file1> <in-file2> -o <out-file2>\n";
        std::cerr << "   mpp -x -include defvar.h \"in.nc\" -out \"out.ncs\"\n";
        std::cerr << "   mpp -verbose -force -map .ncs=.fst -map .ncs=.nc -include defs.def *.ncs\n";
        std::cerr << "       -i[nclude]: include definitions file\n";
        std::cerr << "       -f[orce]: Overwrite file if existing\n";
        std::cerr << "       -m[ap]: map automatic output extension\n";
        std::cerr << "       -o[ut]: output file\n";
        std::cerr << "       -v[erbose]: Print more info\n";
        std::cerr << "       -x|-invert: Invert the dictionary\n";
       }

    const auto& defines() const noexcept { return i_defines; }
    auto& paths_pairs() noexcept { return i_paths_pairs; }
    const auto& outexts_map() const noexcept { return i_outexts_map; }

    bool verbose() const noexcept { return i_verbose; }
    bool can_overwrite() const noexcept { return i_overwrite; }
    bool must_invert_dict() const noexcept { return i_invert_dict; }


    //---------------------------------------------------------------------------
    void invert_extensions_map()
       {
        decltype(i_outexts_map) m_inv;
        for( const auto& [key, value]: i_outexts_map )
           {
            // Handle aliases
            if( auto has = m_inv.find(value); has!=m_inv.end() ) throw dlg::error("Cannot invert extensions, multiple values for {}", has->first);
            m_inv[value] = key;
           }
        // Finally, assign the inverted map
        i_outexts_map = m_inv;
       }


 private:
    std::vector<fs::path> i_defines;
    struct inout_pair_t { fs::path in, out; };
    std::vector<inout_pair_t> i_paths_pairs;
    std::map<std::string,std::string> i_outexts_map = { {".fst", ".ncs"},
                                                        {".nc", ".ncs"}  };
    bool i_verbose = false;
    bool i_overwrite = false;
    bool i_invert_dict = false;
};


//---------------------------------------------------------------------------
inline void build_dictionary(Dictionary& dict, Arguments& args)
{
    for( const auto& def_pth : args.defines() )
       {
        if( args.verbose() ) std::cout << "Adding to dict: " << def_pth << '\n';
        std::vector<std::string> parse_issues;
        dict.load_file(def_pth, parse_issues);
        if( args.verbose() ) std::cout << "    dict: " << dict.to_str() << '\n';

        if( !parse_issues.empty() )
           {
            for(const std::string& issue_txt : parse_issues)
               {
                std::cerr << "    [!] "sv << issue_txt << '\n';
               }
           }
       }
    if( args.must_invert_dict() )
       {
        if( args.verbose() ) std::cout << "Inverting dictionary (" << dict.size() << " entries)\n";
        dict.remove_numbers();
        dict.invert();
        if( args.verbose() ) std::cout << "Inverted dictionary (" << dict.size() << " entries)\n";
        // Also the output file extension mapping (I'm inverting the preprocessing)
        args.invert_extensions_map();
       }
    else
       {
        if( args.verbose() ) std::cout << "Dictionary ready (" << dict.size() << " entries)\n";
       }
  #ifdef _DEBUG
    // Peek dictionary content
    if( args.verbose() )
       {
        std::size_t i = 0;
        for( const auto& [key, value]: dict )
           {
            std::cout << "    <define> " << key << "=" << value << '\n';
            if( i++ > 10 )
               {
                std::cout << "...\n";
                break;
               }
           }
       }
  #endif
}

//---------------------------------------------------------------------------
inline void preprocess_files(const Dictionary& dict, Arguments& args)
{
    for( auto& [in_pth,out_pth] : args.paths_pairs() )
       {
        if( out_pth.empty() )
           {// Must build the output file name
            const std::string ext {str::tolower(in_pth.extension().string())};
            const auto& exts_map = args.outexts_map();
            auto ie = exts_map.find(ext);
            if( ie!=exts_map.end() )
               {// Got a corresponding entry in mapped extension
                //out_pth=in_pth; out_pth.replace_extension(ie->second);
                out_pth = in_pth.parent_path() / in_pth.stem() / ie->second;
               }
            else
               {// Inventing the transformed file name
                out_pth = in_pth.parent_path() / "~";
                out_pth += in_pth.filename();
               }
           }
        // Perform operation
        auto res = pre::process(in_pth, out_pth, dict.map(), args.can_overwrite());
        if( args.verbose() ) std::cout << "  Expanded " << res.n_sub << " macros checking a total of " << res.n_tok << " tokens in " << res.n_line << " lines\n";
        //if(ext==".fst") Mark the file as compiled, setting the DateLastModified = DateCreated
       }
}


//---------------------------------------------------------------------------
int main( int argc, const char* argv[] )
{
    std::ios_base::sync_with_stdio(false); // Better performance

    try{
        Arguments args(argc, argv);

        if( args.verbose() )
           {
            std::cout << "**** mpp (" << __DATE__ << ") ****\n"; // sys::human_readable_time_stamp()
            std::cout << "Running in: " << fs::current_path().string() << '\n';
           }

        if( args.defines().empty() ) throw std::invalid_argument("No defines passed");
        if( args.paths_pairs().empty() ) throw std::invalid_argument("No files passed");

        Dictionary dict;
        build_dictionary(dict, args);

        preprocess_files(dict, args);

        return 0; // EXIT_SUCCESS
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

    return -1; // EXIT_FAILURE
}
