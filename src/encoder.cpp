//  Author:  Noah Himed
//  Date:    23 March 2020
//  Summary: Implements Huffman Encoding Algorithm through
//           the implementation of the Encoder class

#include "encoder.h"
#include "constants.h"

#include <vector>
#include <fstream>
#include <bitset>
#include <string>
#include <exception>

Encoder::Encoder(std::string in_path)
{
    m_tree = nullptr;
    m_in_path = in_path;
    m_file_ext = m_in_path.substr(m_in_path.length()-FILE_EXT_LEN, FILE_EXT_LEN);
    if(m_file_ext != COMPRESSED_FILE_EXT && m_file_ext != ORIGINAL_FILE_EXT)
        throw std::invalid_argument("Invalid argument: please enter files with the entension \""
                                    + COMPRESSED_FILE_EXT + "\" or \"" + ORIGINAL_FILE_EXT + "\"");
}

Encoder::~Encoder()
{
    DeleteTree(m_tree);
    
    // run this block if compress() was ran
    if (m_decomp_if.is_open())
    {
        m_decomp_if.close();
        m_comp_of.close();
    }
    // run this block if decompress() was ran
    if (m_comp_if.is_open())
    {
        m_comp_if.close();
        m_decomp_of.close();
    }
    
}

void Encoder::Compress()
{
    m_decomp_if.open(m_in_path, std::ifstream::in);
    
    // check if file exists
    if(!m_decomp_if.is_open())
        throw std::ios_base::failure("File Error: Unable to open file with given path");
    
    // go through file and calculate char frequencies
    char next_char;
    while(m_decomp_if.get(next_char))
    {
        Node new_leaf(next_char);
        // add a new leaf if the character isn't already in the map
        if (!m_known_chars.emplace(next_char, new_leaf).second)
            // if the char is already in the map, increase frequency
            m_known_chars[next_char].m_freq++;
    }
    
    // go through map, insert into a priority queue to order by frequency
    // a priority queue is used here instead of a map because insertion runtime
    // in a max-heap is better than that of a red black tree
    for (const auto& leafIt : m_known_chars)
        m_leaves.push(new Node(leafIt.second));
    
    // add leaves into queue
    Node *leftPtr, *rightPtr, *parentPtr;
    while (m_leaves.size() > 1)
    {
        leftPtr = new Node(*m_leaves.top());
        m_leaves.pop();
        rightPtr = new Node(*m_leaves.top());
        m_leaves.pop();
        parentPtr = new Node(leftPtr, rightPtr);
        
        m_leaves.push(parentPtr);
    }
    m_tree = m_leaves.top();
    
    // create new encodings, record them in a map
    CreateEncodings(m_tree);
    
    // create new compressed file
    m_out_path = m_in_path.substr(0, m_in_path.length()-FILE_EXT_LEN);
    m_out_path += COMPRESSED_FILE_EXT;
    
    m_comp_of.open(m_out_path, std::fstream::out);
    if(!m_comp_of.is_open())
        throw std::ios_base::failure("File Error: Unable to write to output file");
    
    // write encoded instructions to file
    char encoded_char;
    char code_len;
    std::string huffman_code;
    for (const auto& encoding_instructions : m_encodings)
    {
        encoded_char = encoding_instructions.first;
        code_len = encoding_instructions.second.first;
        huffman_code = encoding_instructions.second.second;
        
        m_comp_of << encoded_char << code_len;
        // some encodings may take up to two bytes
        std::string byte_buffer = GetByteStr(huffman_code);
        for (int i = 0; i < int(byte_buffer.size()); ++i)
            m_comp_of << byte_buffer[i];
    }
    
    // write a null byte to separate decoding instructions from encoded information
    m_comp_of << NULL_BYTE;
    
    // encode information to file
    // restart at start of file
    m_decomp_if.clear();
    m_decomp_if.seekg(0, std::ios::beg);
    std::string encoded_data;
    while (m_decomp_if.get(next_char))
    {
        encoded_data += m_encodings[next_char].second;
        if(encoded_data.size() >= CHAR_LEN_BITS)
        {
            char encoded_byte = GetByteStr(encoded_data.substr(0, CHAR_LEN_BITS))[0];
            m_comp_of << encoded_byte;
            encoded_data = encoded_data.substr(CHAR_LEN_BITS, encoded_data.length()-(CHAR_LEN_BITS-1));
        }
    }
    
    // take care of leftover bits
    if (encoded_data != "")
    {
        int num_bits_empty = CHAR_LEN_BITS - int(encoded_data.size());
        for(int i = 0; i < num_bits_empty; ++i)
            encoded_data += '0';
    }
    
    char last_encoded_byte = GetByteStr(encoded_data)[0];
    m_comp_of << last_encoded_byte;
}

void Encoder::Decompress()
{
    m_comp_if.open(m_in_path, std::ifstream::in);
    
    // check if file exists
    if(!m_comp_if.is_open())
        throw std::ios_base::failure("File Error: Unable to write to output file");
    
    char next_char, key;
    unsigned int code_len, code_width;
    int code_len_signed;
    std::string compression_code;
    
    while (m_comp_if.get(next_char))
    {
        // indicates we've come to the end of the instructions for decompression
        if(next_char == NULL_BYTE)
            break;
        
        key = next_char;
        
        // get number of bytes of next instruction
        if(!m_comp_if.get(next_char))
            throw std::ios_base::failure("File Error: Unable to parse input file correctly");
        // code_len is the actual number of bits in the huffman code, while
        // code_width is the number of bytes the huffman code takes up
        code_len = static_cast<unsigned int>(next_char);
        code_len_signed = static_cast<int>(code_len);
        code_width = 0;
        while (code_len_signed > 0)
        {
            code_len_signed -= CHAR_LEN_BITS;
            code_width++;
        }
        
        // load in huffman code stored as ascii chars from file
        for (int i = 0; i < code_width; ++i)
        {
            if(!m_comp_if.get(next_char))
                throw std::ios_base::failure("File Error: Unable to parse input file correctly");
            
            compression_code += next_char;
        }
        
        // use code_len and compression_code to get byte string corresponding to collected ascii chars
        std::string code_byte_str = GetBitStr(code_len, compression_code);
        
        // store byte string and matching char into a hash map
        if (!m_decomp_map.insert({code_byte_str, key}).second)
            throw std::invalid_argument("Invalid Argument: Encountered duplicate key when attempting to insert into hashmap");
        
        m_huffman_codes.push_back(code_byte_str);
    }
    
    
    // create new decompressed file
    m_out_path = m_in_path.substr(0, m_in_path.length()-FILE_EXT_LEN);
    m_out_path += ORIGINAL_FILE_EXT;

    m_decomp_of.open(m_out_path, std::fstream::out);
    if(!m_decomp_of.is_open())
        throw std::ios_base::failure("File Error: Unable to write to output file");
    
    std::string file_bit_str;
    std::string bit_accumulator;
    auto tree_info_it = m_decomp_map.begin();
    
    // Use the gathered encodings to convert the compressed file’s content back to the original characters.
    while (m_comp_if.get(next_char))
    {
        file_bit_str += std::bitset<CHAR_LEN_BITS>(int(next_char)).to_string();
        
        for (int i = 0; i < file_bit_str.size(); ++i)
        {
            bit_accumulator += file_bit_str[0];
            
            // remove first char from file_bit_str
            if (file_bit_str.size() > 1)
                file_bit_str = file_bit_str.substr(1, file_bit_str.size() - 1);
            else
                file_bit_str = "";
            
            tree_info_it = m_decomp_map.find(bit_accumulator);
            // write decoded char to file
            if (tree_info_it != m_decomp_map.end())
            {
                m_decomp_of << m_decomp_map[bit_accumulator];
                bit_accumulator = "";
            }
        }
    }
    
}

std::string Encoder::GetFileExt() const
{
    return m_file_ext;
}

void Encoder::CreateEncodings(Node *root, std::string code)
{
    if(root->m_ch != NULL_BYTE)
    {
        // add huffman code with corresponding char into encodings
        unsigned int code_len = static_cast<unsigned int>(code.size());
        char len_ch = static_cast<char>(code_len);
        std::pair<char, std::string> encodingBuff(len_ch, code);
        m_encodings.emplace(root->m_ch, encodingBuff);
    }
    else
    {
        // recurse through huffman tree
        CreateEncodings(root->m_left, code+'0');
        CreateEncodings(root->m_right, code+'1');
    }
}

std::string Encoder::GetByteStr(const std::string& str) const
{
    std::string byte_string;
    char currByte = NULL_BYTE;
    for(int i = 0; i < str.size(); i++)
    {
        if(str[i] != '0' && str[i] != '1')
            throw std::runtime_error("Runtime Error: Unable to convert string to byte");
        else
            currByte = currByte << 1 | str[i]-'0';
        
        // add to vector when a full byte has been encoded
        if ((i+1) % CHAR_LEN_BITS == 0)
        {
            byte_string += currByte;
            currByte = NULL_BYTE;
        }
    }
    
    int str_len = static_cast<int>(str.size());
    // left shift the output string so that the first bit is the most significant bit
    if (str_len % CHAR_LEN_BITS != 0)
    {
        if (str_len > CHAR_LEN_BITS)
        {
            while (str_len > CHAR_LEN_BITS)
                str_len -= CHAR_LEN_BITS;
        }
        int num_trailing_zeros = CHAR_LEN_BITS - str_len;
        currByte = currByte << num_trailing_zeros;
        byte_string += currByte;
    }
    
    return byte_string;
}

std::string Encoder::GetBitStr(const unsigned int& len, std::string& chars) const
{
    std::string out;
    char current_byte = chars[0];
    char next_bit;
    int num_byte = 0;
    
    for (unsigned int i = 0; i < len; ++i)
    {
        next_bit = (current_byte & 0b10000000) ? '1' : '0';
        current_byte = current_byte << 1;
        out += next_bit;
        
        // advance to next byte
        if ((i+1) % CHAR_LEN_BITS == 0)
        {
            num_byte++;
            current_byte = chars[num_byte];
        }
    }
    
    // reset string for next use
    chars = "";
    return out;
}

void Encoder::DeleteTree(Node *root)
{
    if (root != nullptr)
    {
        DeleteTree(root->m_left);
        DeleteTree(root->m_right);
        delete root;
    }
}
