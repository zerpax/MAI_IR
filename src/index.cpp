#include <iostream>
#include <vector>
#include <string>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <fstream>


struct PostingList {
    std::vector<int> doc_ids;   
};

struct TermEntry {
    std::string term;           
    int collection_tf;          
    PostingList postings;
};


class BooleanIndex {
private:
    std::vector<std::vector<TermEntry>> table;
    size_t table_size;
    
    size_t hash(const std::string& s) const;
    int total_docs = 0;
public:
    BooleanIndex();
    
    BooleanIndex(size_t size);

    void add_document(int doc_id, const std::vector<std::string>& tokens);

    const std::vector<int>* get_postings(const std::string& term) const;

    int get_collection_tf(const std::string& term) const;
    
    std::vector<int> evaluate_query(const std::vector<std::string>& query_tokens) const;
    
    void save(const std::string& path);
    
    void load(const std::string& path);

};

BooleanIndex::BooleanIndex()
    : table(100000), table_size(100000) {}

BooleanIndex::BooleanIndex(size_t size)
    : table(size), table_size(size) {}

size_t BooleanIndex::hash(const std::string& s) const {
    const size_t p = 31;
    size_t h = 0;

    for (unsigned char c : s) {
        h = h * p + c;
    }

    return h % table_size;
}

void BooleanIndex::add_document(int doc_id,
                                const std::vector<std::string>& tokens) {
    if (doc_id + 1 > total_docs)
        total_docs = doc_id + 1;

    for (const std::string& token : tokens) {
        size_t idx = hash(token);
        std::vector<TermEntry>& bucket = table[idx];

        bool found = false;

        for (TermEntry& entry : bucket) {
            if (entry.term == token) {
                entry.collection_tf++;

                if (entry.postings.doc_ids.empty() ||
                    entry.postings.doc_ids.back() != doc_id) {
                    entry.postings.doc_ids.push_back(doc_id);
                }

                found = true;
                break;
            }
        }

        if (!found) {
            TermEntry new_entry;
            new_entry.term = token;
            new_entry.collection_tf = 1;
            new_entry.postings.doc_ids.push_back(doc_id);

            bucket.push_back(new_entry);
        }
    }
}


const std::vector<int>* BooleanIndex::get_postings(
        const std::string& term) const {

    size_t idx = hash(term);
    const std::vector<TermEntry>& bucket = table[idx];

    for (const TermEntry& entry : bucket) {
        if (entry.term == term) {
            return &entry.postings.doc_ids;
        }
    }

    return nullptr;
}


int BooleanIndex::get_collection_tf(const std::string& term) const {

    size_t idx = hash(term);
    const std::vector<TermEntry>& bucket = table[idx];

    for (const TermEntry& entry : bucket) {
        if (entry.term == term) {
            return entry.collection_tf;
        }
    }

    return 0;
}

std::vector<int> boolean_and(const std::vector<int>& a,
                             const std::vector<int>& b) {
    std::vector<int> result;
    size_t i = 0, j = 0;

    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            result.push_back(a[i]);
            i++;
            j++;
        } else if (a[i] < b[j]) {
            i++;
        } else {
            j++;
        }
    }

    return result;
}

std::vector<int> boolean_or(const std::vector<int>& a,
                            const std::vector<int>& b) {
    std::vector<int> result;
    size_t i = 0, j = 0;

    while (i < a.size() || j < b.size()) {
        if (i < a.size() && (j >= b.size() || a[i] < b[j])) {
            result.push_back(a[i++]);
        } else if (j < b.size() && (i >= a.size() || b[j] < a[i])) {
            result.push_back(b[j++]);
        } else {
            result.push_back(a[i]);
            i++;
            j++;
        }
    }

    return result;
}

std::vector<int> boolean_not(const std::vector<int>& a,
                             int total_docs) {
    std::vector<int> result;
    size_t j = 0;

    for (int doc = 0; doc < total_docs; doc++) {
        if (j < a.size() && a[j] == doc) {
            j++;
        } else {
            result.push_back(doc);
        }
    }

    return result;
}

std::vector<int> BooleanIndex::evaluate_query(
        const std::vector<std::string>& query_tokens) const {
    
    if (query_tokens.size() == 1) {
        auto* p = get_postings(query_tokens[0]);
        return p ? *p : std::vector<int>();
    }

    if (query_tokens.size() == 2 && query_tokens[0] == "not") {
        auto* p = get_postings(query_tokens[1]);
        if (!p) return {};
        return boolean_not(*p, total_docs);
    }

    if (query_tokens.size() == 3) {
        auto* left = get_postings(query_tokens[0]);
        auto* right = get_postings(query_tokens[2]);

        if (!left || !right) return {};

        if (query_tokens[1] == "and")
            return boolean_and(*left, *right);

        if (query_tokens[1] == "or")
            return boolean_or(*left, *right);
    }

    return {};
}

void BooleanIndex::save(const std::string& path)  {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open file for saving index");
    }

    // Global metadata
    out.write(reinterpret_cast<const char*>(&table_size), sizeof(table_size));
    out.write(reinterpret_cast<const char*>(&total_docs), sizeof(total_docs));

    // Hash table
    for (const auto& bucket : table) {
        int bucket_size = static_cast<int>(bucket.size());
        out.write(reinterpret_cast<const char*>(&bucket_size), sizeof(bucket_size));

        for (const auto& entry : bucket) {
            // term
            int term_len = static_cast<int>(entry.term.size());
            out.write(reinterpret_cast<const char*>(&term_len), sizeof(term_len));
            out.write(entry.term.data(), term_len);

            // collection term frequency
            out.write(reinterpret_cast<const char*>(&entry.collection_tf),
                      sizeof(entry.collection_tf));

            // postings
            int pcount = static_cast<int>(entry.postings.doc_ids.size());
            out.write(reinterpret_cast<const char*>(&pcount), sizeof(pcount));
            out.write(reinterpret_cast<const char*>(entry.postings.doc_ids.data()),
                      pcount * sizeof(int));
        }
    }
}


void BooleanIndex::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open file for loading index");
    }

    // Clear existing index
    table.clear();

    // Global metadata
    in.read(reinterpret_cast<char*>(&table_size), sizeof(table_size));
    in.read(reinterpret_cast<char*>(&total_docs), sizeof(total_docs));

    table.resize(table_size);

    // Hash table
    for (size_t i = 0; i < table_size; ++i) {
        int bucket_size;
        in.read(reinterpret_cast<char*>(&bucket_size), sizeof(bucket_size));

        table[i].reserve(bucket_size);

        for (int j = 0; j < bucket_size; ++j) {
            TermEntry entry;

            // term
            int term_len;
            in.read(reinterpret_cast<char*>(&term_len), sizeof(term_len));
            entry.term.resize(term_len);
            in.read(&entry.term[0], term_len);

            // collection term frequency
            in.read(reinterpret_cast<char*>(&entry.collection_tf),
                    sizeof(entry.collection_tf));

            // postings
            int pcount;
            in.read(reinterpret_cast<char*>(&pcount), sizeof(pcount));
            entry.postings.doc_ids.resize(pcount);
            in.read(reinterpret_cast<char*>(entry.postings.doc_ids.data()),
                    pcount * sizeof(int));

            table[i].push_back(std::move(entry));
        }
    }
}


namespace py = pybind11;

PYBIND11_MODULE(index, m) {
    m.doc() = "Boolean inverted index implemented in C++";

    py::class_<BooleanIndex>(m, "BooleanIndex")
        .def(py::init<>())    

        .def(py::init<size_t>(),
             py::arg("table_size"))

        .def("add_document",
             &BooleanIndex::add_document,
             py::arg("doc_id"),
             py::arg("tokens"))

        .def("get_postings",
             &BooleanIndex::get_postings,
             py::return_value_policy::reference_internal,
             py::arg("term"))

        .def("get_collection_tf",
             &BooleanIndex::get_collection_tf,
             py::arg("term"))

        .def("evaluate_query",
             &BooleanIndex::evaluate_query,
             py::arg("query_tokens"))
        
        .def("save", 
            &BooleanIndex::save,
            py::arg("path"))

        .def("load", 
            &BooleanIndex::load,
            py::arg("path"));
}
