#ifndef dictionary_hpp
#define dictionary_hpp
//  ---------------------------------------------
//  Dictionary facilities
//  ---------------------------------------------
#include "poor-mans-unicode.hpp" // enc::Bom
#include "def-parser.hpp" // def::parse
#include "h-parser.hpp" // h::parse
#include "logging.hpp" // dlg::error
#include "system.hpp" // sys::*, fs::*
#include "string-utilities.hpp" // 'str::tolower
#include <string>
#include <map>
#include <cctype> // std::isdigit, ...
#include <fstream> // std::*fstream



/////////////////////////////////////////////////////////////////////////////
// key-value string pairs
class Dictionary
{
 public:
    typedef std::map<std::string,std::string> container_type;
    typedef container_type::size_type size_type;
    typedef container_type::value_type value_type; // pair
    typedef container_type::key_type key_type;
    typedef container_type::mapped_type mapped_type;
    typedef container_type::iterator iterator;
    typedef container_type::const_iterator const_iterator;


    //---------------------------------------------------------------------------
    void load_file(const fs::path& pth, std::vector<std::string>& issues)
       {
        const bool fussy = true;
        const std::string ext {str::tolower(pth.extension().string())};
        if( ext == ".plc" )
           {// That's the 'DEF' syntax used in fagor sources (// DEF macro expansion ; comment)

            std::ifstream is( pth, std::ios::binary );
            if( !is ) throw dlg::error("Cannot read {}", pth.string());

            // Potrebbe avere varie codifiche
            //const enc::Bom bom(is);
            //     if( bom.is_utf16_le() ) def::parse<char16_t,true>(is, i_map, issues, fussy);
            //else if( bom.is_utf16_be() ) def::parse<char16_t,false>(is, i_map, issues, fussy);
            //else if( bom.is_utf32_le() ) def::parse<char32_t,true>(is, i_map, issues, fussy);
            //else if( bom.is_utf32_be() ) def::parse<char32_t,false>(is, i_map, issues, fussy);
            //else                         def::parse<char,false>(is, i_map, issues, fussy); // bom.is_ansi() || bom.is_utf8()
           }
        else
           {// An header file (#define macro expansion // comment)
            // Supporto solo codifica utf-8
            sys::MemoryMappedFile buf( pth.string() );

            std::vector<h::Define> defs;
            h::parse(buf.as_string_view(), defs, issues, fussy);

            // Add the defines to dictionary
            for( const auto& def : defs )
               {
                insert_unique( def.label(), def.value() );
               }
            //for( auto i=defs.cbegin(); i!=defs.cend(); ++i )
            //   {
            //    insert_unique( i->label(), i->value() );
            //   }
           }
       }

    //---------------------------------------------------------------------------
    void remove_numbers()
       {
        const_iterator i = i_map.begin();
        while( i != i_map.end() )
           {
            if (is_num(i->second)) i = i_map.erase(i);
            else ++i;
           }
       }

    //---------------------------------------------------------------------------
    // Invert the dictionary (you should exclude numbers)
    void invert()
       {
        container_type inv_dict;
        //inv_dict.reserve( size() );

        for( const auto& [key, value]: i_map )
           {
            // Handle aliases, the correct one must be manually chosen
            if( auto has = inv_dict.find(value);
                has != inv_dict.end() )
               {// Already existing, add the alias
                // Add a recognizable placeholder to ease the manual choose: <CHOOSE:alias1|alias2|alias3>
                if(has->second.find("<CHOOSE:")==std::string::npos) has->second = fmt::format("<CHOOSE:{}>",has->second);
                has->second[has->second.length()-1] = '|'; // *(has->second.rbegin()) = '|';
                has->second += key;
                has->second += '>';
               }
            else
               {
                auto ins = inv_dict.emplace(value, key);
                if( !ins.second ) throw dlg::error("Cannot insert \"{}\" inverting dictionary", value);
               }
           }

        i_map = inv_dict;
       }


    std::string to_str()
       {
        return fmt::format("{} defines", size());
       }

    size_type size() const noexcept { return i_map.size(); }
    bool is_empty() const noexcept { return i_map.empty(); }
    //bool has(const key_type& k) const noexcept { return i_map.find(k)!=i_map.end(); }

    const_iterator find(const key_type& k) const { return i_map.find(k); }
    iterator find(const key_type& k) { return i_map.find(k); }

    // Avoiding operator[]
    //const mapped_type& operator[](const key_type& k) const noexcept { return i_map[k]; }
    //mapped_type& operator[](const key_type& k) noexcept { return i_map[k]; }

    //const mapped_type& get(const key_type& k, const mapped_type& def) const
    //   {
    //    const_iterator i = i_map.find(k);
    //    if(i!=i_map.end()) return i->second;
    //    else return def;
    //   }
    //const mapped_type& get(const key_type& k) const
    //   {
    //    const_iterator i = i_map.find(k);
    //    if(i!=i_map.end()) return i->second;
    //    else throw dlg::error("key \'{}\' not found in dictionary", k);
    //   }
    //mapped_type& get(const key_type& k)
    //   {
    //    iterator i = i_map.find(k);
    //    if(i!=i_map.end()) return i->second;
    //    else throw dlg::error("key \'{}\' not found in dictionary", k);
    //   }

    mapped_type& insert_if_missing(const std::string_view k, const std::string_view v)
       {
        std::pair<container_type::iterator,bool> ins = i_map.emplace(k,v);
        return ins.first->second;
       }
    mapped_type& insert_if_missing(const key_type& k, const mapped_type& v)
       {
        std::pair<container_type::iterator,bool> ins = i_map.insert( value_type(k,v) );
        return ins.first->second;
       }

    mapped_type& insert_or_assign(const std::string_view k, const std::string_view v)
       {
        std::pair<container_type::iterator,bool> ins = i_map.emplace(k,v);
        if( !ins.second )
           {// Key already existing, value not inserted: overwrite
            ins.first->second = v;
           }
        return ins.first->second;
       }
    mapped_type& insert_or_assign(const key_type& k, const mapped_type& v)
       {
        std::pair<container_type::iterator,bool> ins = i_map.insert( value_type(k,v) );
        if( !ins.second )
           {// Key already existing, value not inserted: overwrite
            ins.first->second = v;
           }
        return ins.first->second;
       }

    mapped_type& insert_unique(const std::string_view k, const std::string_view v)
       {
        std::pair<container_type::iterator,bool> ins = i_map.emplace(k,v);
        if(!ins.second) throw dlg::error("Key \'{}\' already present in dictionary",k);
        return ins.first->second;
       }
    mapped_type& insert_unique(const key_type& k, const mapped_type& v)
       {
        std::pair<container_type::iterator,bool> ins = i_map.insert( value_type(k,v) );
        if(!ins.second) throw dlg::error("Key \'{}\' already present in dictionary",k);
        return ins.first->second;
       }

    //iterator erase(const_iterator i) { return i_map.erase(i); }
    //size_type erase(const key_type& k) { return i_map.erase(k); }
    //void clear() { i_map.clear(); }

    const_iterator begin() const noexcept { return i_map.begin(); }
    const_iterator end() const noexcept { return i_map.end(); }
    iterator begin() noexcept { return i_map.begin(); }
    iterator end() noexcept { return i_map.end(); }

    const container_type& map() const noexcept { return i_map; };

 private:
    container_type i_map;

    static bool is_num(const mapped_type& v) noexcept
       {
        if( v.length()<=0 ) return false;
        if( std::isdigit(v[0]) ) return true;
        if( v.length()<=1 ) return false;
        if( v[0]=='-' || v[0]=='+' || v[0]=='.' ) return std::isdigit(v[1]);
        return false;
       }
};



//---------------------------------------------------------------------------
// Invert a map
//template<typename K,typename V> void invert_map(std::map<K,V>& m, const bool strict)
//{
//    std::map<K,V> m_inv;
//    for( const auto& [key, value]: m )
//       {
//        // Handle aliases
//        auto has = m_inv.find(value);
//        if( has != m_inv.end() )
//           {// Already existing
//            if(strict) throw dlg::error("Cannot invert map, multiple values for {}",has->first);
//            //else has->second = key; // Take last instead of keeping the first
//           }
//        else
//           {
//            m_inv[value] = key;
//            //auto ins = m_inv.emplace(value, key);
//            //if( !ins.second ) throw dlg::error("Cannot invert map, \'{}\' not inserted",value);
//           }
//       }
//   // Finally, assign the inverted map
//   m = m_inv;
//}


//---- end unit -------------------------------------------------------------
#endif
