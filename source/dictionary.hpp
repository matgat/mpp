#ifndef dictionary_hpp
#define dictionary_hpp
/*  ---------------------------------------------
    Dictionary facilities
    --------------------------------------------- */
    #include <string>
    #include <map>
    #include <iostream> // 'std::cerr'
    #include <fstream> // 'std::ifstream'


using dict_t = std::map<std::string,std::string>;


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
        else
           {
            m_inv[i->second] = i->first;
            //auto ins = m_inv.insert( std::map<K,V>::value_type( i->second, i->first ) );
            //if( !ins.second ) throw std::runtime_error("cannot invert map, cannot insert " + i->second);
           }
       }
   // Finally, assign the inverted map
   m = m_inv;
}


//---------------------------------------------------------------------------
int parse(dict_t& dict, const std::string& pth, const bool verbose )
{
    // (0) Open file for read
    if(verbose) std::cout << "\n[" << pth << "]\n";
    std::ifstream fin( pth, std::ios::binary );
    if( !fin )
       {
        std::cerr << "  Cannot read " << pth << '\n';
        return 1;
       }

    // (1) See syntax (extension) and parse
    std::string dir,nam,ext;
    mat::split_path(pth, dir,nam,ext);
    bool fagor = (enc::tolower(ext)==".plc");

    // (2) Parse the file
    switch( enc::check_bom(fin, nullptr, verbose) )
       {
        case enc::ANSI :
        case enc::UTF8 :
            if(fagor) return Parse_D<char>(*this, fin, verbose);
            else return Parse_H<char>(*this, fin, verbose);

        case enc::UTF16_LE :
            if(fagor) return Parse_D<char16_t,true>(*this, fin, verbose);
            return Parse_H<char16_t,true>(*this, fin, verbose);

        case enc::UTF16_BE :
            if(fagor) return Parse_D<char16_t,false>(*this, fin, verbose);
            return Parse_H<char16_t,false>(*this, fin, verbose);

        //case enc::UTF32_LE :
        //    if(fagor) return Parse_D<char32_t,true>(*this, fin, verbose);
        //    return Parse_H<char32_t,true>(*this, fin, verbose);

        //case enc::UTF32_BE :
        //    if(fagor) return Parse_D<char32_t,false>(*this, fin, verbose);
        //    return Parse_H<char32_t,false>(*this, fin, verbose);

        default :
            std::cerr << "  Cannot handle the encoding of " << pth << "\n";
            return 1;
       }
}

//---------------------------------------------------------------------------
// Invert the dictionary (can exclude numbers)
dict_t invert(const dict_t& dict, const bool nonum, const bool verbose)
{
    if(verbose) std::cout << "\n{Inverting dictionary}\n";
    if(nonum) std::cerr << "  Excluding numbers\n";
    dict_t inv_map;
    for( const auto& [key, value]: dict )
       {
        if( nonum )
           {// Exclude numbers
            try{ mat::ToNum(value); continue; } // Skip the number
            catch(...) {} // Ok, not a number
           }
        // Handle aliases, the correct one must be manually chosen
        auto has = inv_map.find(value);
        if( has != inv_map.end() )
           {// Already existing, add the alias
            //std::cerr << "  Adding alias \'" << key << "\' for " << value << '\n';
            // Add a recognizable placeholder to ease the manual choose
            if(has->second.find("<CHOOSE:")==std::string::npos) has->second = "<CHOOSE:" + has->second + ">";
            has->second[has->second.length()-1] = '|'; // *(has->second.rbegin()) = '|';
            has->second += key + ">";
           }
        else
           {
            auto ins = inv_map.insert( value_type( value, key ) );
            if( !ins.second )
               {
                std::cerr << "  Cannot insert \'" << value << "\' !!\n";
               }
           }
       }
    if(verbose) std::cout << "  Now got " << inv_map.size() << " voices from previous " << size() << '\n';
    // Finally, assign the inverted dictionary
    return inv_map;
}


//void peek(const dict_t& dict)
//   {
//    int max = 200;
//    for( const auto& [key, value]: dict )
//       {
//        std::cout << "  " << key << " " << value << '\n';
//        if(--max<0) { std::cout << "...\n"; break; }
//       }
//   }


//---- end unit -------------------------------------------------------------
#endif
