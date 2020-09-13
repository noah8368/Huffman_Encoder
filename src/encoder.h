//  Author:  Noah Himed
//  Date:    23 March 2020
//  Summary: Defines Encoder class

#ifndef HuffmanEncoding_h
#define HuffmanEncoding_h

#include "constants.h"

#include <string>
#include <unordered_map>
#include <queue>
#include <vector>
#include <utility>
#include <exception>
#include <fstream>

class Encoder
{
public:
    Encoder(std::string in_path);
    ~Encoder();
    // compress a text file, creates a compressed huf file
    void Compress();
    // decompresses a huf file, creates a text file
    void Decompress();
    std::string GetFileExt() const;
private:
    struct Node
    {
        Node()
        {
            m_ch = NULL_BYTE;
            m_left = m_right = nullptr;
        }
        Node(char ch, int freq)
        {
            m_ch = ch;
            m_freq = freq;
            m_left = m_right = nullptr;
        }
        Node(const Node &src)
        {
            m_ch = src.m_ch;
            m_freq = src.m_freq;
            m_left = src.m_left;
            m_right = src.m_right;
        }
        Node(char ch)
        {
            m_ch = ch;
            m_freq = 1;
            m_left = m_right = nullptr;
        }
        Node(Node *left, Node *right)
        {
            m_ch = NULL_BYTE;
            m_freq = left->m_freq + right->m_freq;
            m_left = left;
            m_right = right;
        }
        
        char m_ch;
        int m_freq;
        Node *m_left, *m_right;
    };
    
    // used to sort nodes in priority queue
    struct NodeComparator
    {
        bool operator()(const Node *lhs, const Node *rhs)
        {
            return lhs->m_freq > rhs->m_freq;
        }
    };
    
    Node* m_tree;
    
    std::ifstream m_decomp_if;
    std::ifstream m_comp_if;
    
    std::ofstream m_decomp_of;
    std::ofstream m_comp_of;

    std::vector<std::string> m_huffman_codes;
    
    std::unordered_map<char, Node> m_known_chars;
    std::unordered_map<char, std::pair<char, std::string>> m_encodings;
    std::unordered_map<std::string, char> m_decomp_map;
    
    std::priority_queue<Node*, std::vector<Node*>, NodeComparator> m_leaves;
    
    std::string m_file_ext;
    std::string m_in_path;
    std::string m_out_path;
    
    // Take in a string of '1' or '0' chars, and convert into a corresponding
    // string of ascii chars
    std::string GetByteStr(const std::string& str) const;
    // Convert string of ascii chars to a string of '1' and '0' chars
    std::string GetBitStr(const unsigned int& len, std::string& chars) const;
    // creates bit sequences for every unique byte in file
    void CreateEncodings(Node *root, std::string code="");
    void DeleteTree(Node *root);
};

#endif /* HuffmanEncoding_h */
