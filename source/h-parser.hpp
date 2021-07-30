#ifndef h_parser_hpp
#define h_parser_hpp
/*  ---------------------------------------------
    ©2021 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    LogicLab 'pll' file format

    DEPENDENCIES:
    --------------------------------------------- */
    #include "logging.hpp" // 'dlg::parse_error', 'dlg::error', 'DBGLOG', 'fmt::format'
    #include "poor-mans-unicode.hpp" // 'enc::Bom'
    #include "string-utilities.hpp" // 'str::escape'
    #include <string_view>
    #include <stdexcept> // 'std::exception', 'std::runtime_error', ...
    #include <cctype> // 'std::isdigit', 'std::isblank', ...

using namespace std::literals; // Use "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace h //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
// Descriptor of a '#define' entry in buffer
class Define ////////////////////////////////////////////////////////////////
{
 public:

    std::string_view label() const noexcept { return i_Label; }
    void set_label(const std::string_view s)
       {
        if(s.empty()) throw dlg::error("Empty define label");
        i_Label = s;
       }

    std::string_view value() const noexcept { return i_Value; }
    void set_value(const std::string_view s)
       {
        if(s.empty()) throw dlg::error("Empty define value");
        i_Value = s;
       }

    std::string_view comment() const noexcept { return i_Comment; }
    void set_comment(const std::string_view s) noexcept { i_Comment = s; }
    bool has_comment() const noexcept { return !i_Comment.empty(); }

 private:
    std::string_view i_Label;
    std::string_view i_Value;
    std::string_view i_Comment;
};


//---------------------------------------------------------------------------
// Parse h file
void parse(const std::string_view buf, Defines& defs, std::vector<std::string>& issues, const bool fussy)
{
    std::size_t i = 0; // Current character

   {// Check BOM
    const enc::Bom bom(buf, i);
    if( !bom.is_ansi() && !bom.is_utf8() ) throw std::runtime_error("Bad encoding, not UTF-8");
   }

    const std::size_t siz = buf.size();
    if( siz < 2 )
       {
        if(fussy)
           {
            throw std::runtime_error("Empty file");
           }
        else
           {
            issues.emplace_back("Empty file");
            return;
           }
       }

    std::size_t line = 1; // Current line number

    //---------------------------------
    auto notify_error = [&](const std::string_view msg, auto... args)
       {
        if(fussy)
           {
            throw dlg::error(msg, args...);
           }
        else
           {
            issues.push_back( fmt::format("{} (line {}, offset {})", fmt::format(msg, args...), line, i) );
           }
       };

    //---------------------------------
    auto is_blank = [](const char c) noexcept -> bool
       {
        return std::isspace(c) && c!='\n';
       };

    //---------------------------------
    auto skip_blanks = [&]() noexcept -> void
       {
        while( i<siz && is_blank(buf[i]) ) ++i;
       };

    //---------------------------------
    auto last_not_blank = [&](std::size_t i_last) noexcept -> std::size_t
       {
        while( i_last>0 && is_blank(buf[i_last-1]) ) --i_last;
        return i_last;
       };

    //---------------------------------
    auto eat_line_end = [&]() noexcept -> bool
       {
        if( buf[i]=='\n' )
           {
            ++i;
            ++line;
            return true;
           }
        return false;
       };

    //---------------------------------
    auto skip_line = [&]() noexcept -> std::string_view
       {
        const std::size_t i_start = i;
        while( i<siz && !eat_line_end() ) ++i;
        return std::string_view(buf.data()+i_start, i-i_start);
       };

    //---------------------------------
    auto eat_line_comment_start = [&]() noexcept -> bool
       {
        if( i<(siz-1) && buf[i]=='/' && buf[i+1]=='/' )
           {
            i += 2; // Skip "//"
            return true;
           }
        return false;
       };

    //---------------------------------
    //auto eat_block_comment_start = [&]() noexcept -> bool
    //   {
    //    if( i<(siz-1) && buf[i]=='/' && buf[i+1]=='*' )
    //       {
    //        i += 2; // Skip "/*"
    //        return true;
    //       }
    //    return false;
    //   };

    //---------------------------------
    //auto skip_block_comment = [&]() -> void
    //   {
    //    const std::size_t line_start = line; // Store current line
    //    const std::size_t i_start = i; // Store current position
    //    const std::size_t sizm1 = siz-1;
    //    while( i<sizm1 )
    //       {
    //        if( buf[i]=='*' && buf[i+1]=='/' )
    //           {
    //            i += 2; // Skip "*/"
    //            return;
    //           }
    //        else if( buf[i]=='\n' )
    //           {
    //            ++line;
    //           }
    //        ++i;
    //       }
    //    throw dlg::parse_error("Unclosed block comment", line_start, i_start);
    //   };

    //---------------------------------
    auto eat_token = [&](const std::string_view s) noexcept -> bool
       {
        const std::size_t i_end = i+s.length();
        if( ((i_end<siz && !std::isalnum(buf[i_end])) || i_end==siz) && s==std::string_view(buf.data()+i,s.length()) )
           {
            i = i_end;
            return true;
           }
        return false;
       };

    //---------------------------------
    auto collect_token = [&]() noexcept -> std::string_view
       {
        const std::size_t i_start = i;
        while( i<siz && !std::isspace(buf[i]) ) ++i;
        return std::string_view(buf.data()+i_start, i-i_start);
       };

    //---------------------------------
    auto collect_identifier = [&]() noexcept -> std::string_view
       {
        const std::size_t i_start = i;
        while( i<siz && (std::isalnum(buf[i]) || buf[i]=='_') ) ++i;
        return std::string_view(buf.data()+i_start, i-i_start);
       };

    //---------------------------------
    auto collect_until_char_trimmed = [&](const char c) -> std::string_view
       {
        const std::size_t i_start = i;
        while( i<siz )
           {
            if( buf[i]==c ) break;
            else if( buf[i]=='\n' ) ++line;
            ++i;
           }
        const std::size_t i_end = last_not_blank(i);
        return std::string_view(buf.data()+i_start, i_end-i_start);
       };

    //---------------------------------
    //auto eat_directive_start = [&]() noexcept -> bool
    //   {//#define
    //    if( i<siz && buf[i]=='#' )
    //       {
    //        ++i;
    //        return true;
    //       }
    //    return false;
    //   };

    //---------------------------------
    auto collect_define = [&]() -> Define
       {// LABEL       0  // [INT] Descr
        // vnName     vn1782  // descr [unit]
        // Contract: '#define' already eat
        Define def;
        // [Label]
        skip_blanks();
        def.set_label( collect_identifier() );
        // [Value]
        skip_blanks();
        def.set_value( collect_token() );
        // [Comment]
        skip_blanks();
        if( eat_line_comment_start() )
           {
            skip_blanks();
            def.set_comment( collect_until_char_trimmed('\n') );
           }
        else
            {
             notify_error("Define {} hasn't a comment", def.label());
            }

        // Expecting a line end now
        skip_blanks();
        if( !eat_line_end() ) notify_error("Unexpected content after define \"{}\": {}", def.label(), str::escape(skip_line()));
        //DBGLOG("    [*] Collected define: label=\"{}\" value=\"{}\" comment=\"{}\"\n", def.label(), def.value(), def.comment())
        return def;
       };


    try{
        while( i<siz )
           {
            skip_blanks();
            if(i>=siz) break;
            else if( eat_line_comment_start() ) skip_line(); // A line comment
            //else if( eat_block_comment_start() ) skip_block_comment(); // If supporting block comments
            else if( eat_line_end() ) continue; // An empty line
            else if( eat_token("#define"sv) )
               {
                defs.push_back( collect_define() );
               }
            else
               {
                notify_error("Unexpected content: {}", str::escape(skip_line()));
               }
           } // while there's data
       }
    catch(std::exception& e)
       {
        throw dlg::parse_error(e.what(), line, i);
       }
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
