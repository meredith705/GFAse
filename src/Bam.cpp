#include "Bam.hpp"

#include <stdexcept>
#include <iostream>
#include <vector>

using std::runtime_error;
using std::vector;
using std::cerr;


namespace gfase{


Bam::Bam(path bam_path):
    bam_path(bam_path),
    bam_file(nullptr),
//    bam_index(nullptr),
    bam_iterator(nullptr)
{
    if ((bam_file = hts_open(bam_path.string().c_str(), "r")) == 0) {
        throw runtime_error("ERROR: Cannot open bam file: " + bam_path.string());
    }

//    // bam index
//    if ((bam_index = sam_index_load(bam_file, bam_path.string().c_str())) == 0) {
//        throw runtime_error("ERROR: Cannot open index for bam file: " + bam_path.string() + "\n");
//    }

    // bam header
    if ((bam_header = sam_hdr_read(bam_file)) == 0){
        throw runtime_error("ERROR: Cannot open header for bam file: " + bam_path.string() + "\n");
    }

    alignment = bam_init1();
}


void Bam::for_alignment_in_bam(const function<void(const string& ref_name, const string& query_name, uint8_t map_quality, uint16_t flag)>& f){
    while (sam_read1(bam_file, bam_header, alignment) >= 0){
        string query_name = bam_get_qname(alignment);

        string ref_name;

        // Ref name field might be empty if read is unmapped, in which case the target (aka ref) id might not be in range
        if (alignment->core.tid < bam_header->n_targets and alignment->core.tid > 0) {
            ref_name = bam_header->target_name[alignment->core.tid];
        }

        f(ref_name, query_name, alignment->core.qual, alignment->core.flag);
    }
}


void Bam::for_alignment_in_bam(bool get_cigar, const function<void(SamElement& alignment)>& f){
    while (sam_read1(bam_file, bam_header, alignment) >= 0){
        SamElement e;
        e.query_name = bam_get_qname(alignment);

        // Ref name field might be empty if read is unmapped, in which case the target (aka ref) id might not be in range
        if (alignment->core.tid < bam_header->n_targets and alignment->core.tid > -1) {
            e.ref_name = bam_header->target_name[alignment->core.tid];
        }

        e.mapq = alignment->core.qual;
        e.flag = alignment->core.flag;

        if (get_cigar) {
            auto n_cigar = alignment->core.n_cigar;
            auto cigar_ptr = bam_get_cigar(alignment);
            e.cigars.assign(cigar_ptr, cigar_ptr + n_cigar);
        }

        f(e);
    }
}


Bam::~Bam() {
    hts_close(bam_file);
    bam_hdr_destroy(bam_header);
    bam_destroy1(alignment);
//    hts_idx_destroy(bam_index);
    hts_itr_destroy(bam_iterator);
}


bool Bam::is_first_mate(uint16_t flag){
    return (uint16_t(flag) >> 6) & uint16_t(1);
}


bool Bam::is_second_mate(uint16_t flag){
    return (uint16_t(flag) >> 7) & uint16_t(1);
}


bool Bam::is_not_primary(uint16_t flag){
    return (uint16_t(flag) >> 8) & uint16_t(1);
}


bool Bam::is_primary(uint16_t flag){
    return (not is_not_primary(flag));
}


bool Bam::is_supplementary(uint16_t flag){
    return (uint16_t(flag) >> 11) & uint16_t(1);
}


void update_contact_map(
        vector<SamElement>& alignments,
        MultiContactGraph& contact_graph,
        IncrementalIdMap<string>& id_map){

    // Iterate one triangle of the all-by-all matrix, adding up mapqs for reads on both end of the pair
    for (size_t i=0; i<alignments.size(); i++){
        auto& a = alignments[i];
        auto ref_id_a = int32_t(id_map.try_insert(a.ref_name));
        contact_graph.try_insert_node(ref_id_a, 0);

        contact_graph.increment_coverage(ref_id_a, 1);

        for (size_t j=i+1; j<alignments.size(); j++) {
            auto& b = alignments[j];
            auto ref_id_b = int32_t(id_map.try_insert(b.ref_name));
            contact_graph.try_insert_node(ref_id_b, 0);
            contact_graph.try_insert_edge(ref_id_a, ref_id_b);
            contact_graph.increment_edge_weight(ref_id_a, ref_id_b, 1);
        }
    }
}


void parse_unpaired_bam_file(
        path bam_path,
        MultiContactGraph& contact_graph,
        IncrementalIdMap<string>& id_map,
        string required_prefix,
        int8_t min_mapq){

    Bam reader(bam_path);

    size_t l = 0;
    string prev_query_name = "";
    vector<SamElement> alignments;

    reader.for_alignment_in_bam(false, [&](const SamElement& a){
        if (l == 0){
            prev_query_name = a.query_name;
        }

        if (prev_query_name != a.query_name){
            update_contact_map(alignments, contact_graph, id_map);
            alignments.clear();
        }

        // No information about reference contig, this alignment is unusable
        if (a.ref_name.empty()){
            return;
        }

        // Optionally filter by the contig names. E.g. "PR" in shasta
        bool valid_prefix = true;
        if (not required_prefix.empty()){
            for (size_t i=0; i<required_prefix.size(); i++){
                if (a.ref_name[i] != required_prefix[i]){
                    valid_prefix = false;
                    break;
                }
            }
        }

        if (valid_prefix) {
            // Only allow reads with mapq > min_mapq and not secondary
            if (a.mapq >= min_mapq and a.is_primary()) {
                alignments.emplace_back(a);
            }
        }

        l++;
        prev_query_name = a.query_name;
    });
}



}
