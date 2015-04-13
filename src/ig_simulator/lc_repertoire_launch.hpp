#include "gene_database.hpp"
#include "base_repertoire_creation/vdj_recombinator.hpp"
#include "base_repertoire_creation/exonuclease_remover.hpp"
#include "base_repertoire_creation/p_nucleotides_creator.hpp"
#include "base_repertoire_creation/n_nucleotides_creator.hpp"
#include "base_repertoire_creation/cdr_labeler.hpp"
#include "repertoire.hpp"
#include "multiplicity_creator.hpp"
//#include "mutated_repertoire_creation/shm_creator.hpp"

struct LC_InputParams {
    string vgenes_fname;
    string jgenes_fname;

    size_t base_repertoire_size;
    size_t mutated_repertoire_size;
    size_t final_repertoire_size;
};

LC_GenesDatabase_Ptr CreateLCDatabase(LC_InputParams params) {
    LC_GenesDatabase_Ptr lc_database(new LC_GenesDatabase());
    lc_database->AddGenesFromFile(variable_gene, params.vgenes_fname);
    lc_database->AddGenesFromFile(join_gene, params.jgenes_fname);
    return lc_database;
}

LC_Repertoire_Ptr CreateBaseLCRepertoire(LC_InputParams params, LC_GenesDatabase_Ptr lc_database) {
    LC_SimpleRecombinator lc_vdj_recombinator(lc_database, LC_SimpleRecombinationCreator(lc_database));

    // endonuclease remover
    LC_SimpleRemovingStrategy removing_strategy;
    LC_SimpleExonucleaseRemover remover(removing_strategy);

    // p nucleotides insertion
    LC_SimplePInsertionStrategy p_nucls_strategy;
    LC_SimplePNucleotidesCreator p_creator(p_nucls_strategy);

    // n nucleotides insertion
    LC_SimpleNInsertionStrategy n_nucls_strategy;
    LC_SimpleNNucleotidesCreator n_creator(n_nucls_strategy);

    // repertoire
    LC_Repertoire_Ptr base_repertoire(new LC_Repertoire());

    // base multiplicity creator
    double base_lambda = double(params.base_repertoire_size) / params.mutated_repertoire_size;
    LC_ExponentialMultiplicityCreator base_multiplicity_creator(base_lambda);

    // cdr labeling
    LC_CDRLabelingStrategy cdr_labeling_strategy;
    LC_CDRLabeler cdr_labeler(cdr_labeling_strategy);

    for(size_t i = 0; i < params.base_repertoire_size; i++) {
        auto vdj = lc_vdj_recombinator.CreateRecombination();
        vdj = remover.CreateRemovingSettings(vdj);
        vdj = p_creator.CreatePNucleotides(vdj);
        vdj = n_creator.CreateNNucleotides(vdj);
        cout << (*vdj);
        cout << "-----------------" << endl;

        LC_VariableRegionPtr ig_variable_region = LC_VariableRegionPtr(new LC_VariableRegion(vdj));
        ig_variable_region = cdr_labeler.LabelCDRs(ig_variable_region);

        size_t multiplicity = base_multiplicity_creator.AssignMultiplicity(ig_variable_region);
        base_repertoire->Add(LC_Cluster(ig_variable_region, multiplicity));
    }
    return base_repertoire;
}

LC_Repertoire_Ptr CreateMutatedLCRepertoire(LC_InputParams params, LC_Repertoire_Ptr base_repertoire) {
    LC_Repertoire_Ptr mutated_repertoire(new LC_Repertoire());

    // mutated multiplicity creator
    double mutated_lambda = double(base_repertoire->NumberAntibodies()) / double(params.final_repertoire_size);
    LC_ExponentialMultiplicityCreator mutated_multiplicity_creator(mutated_lambda);

    // todo!!!
    //size_t min_number_mutations = 2;
    //size_t max_number_mutations = 10;
    //HC_RgywWrcySHMStrategy shm_creation_strategy(min_number_mutations, max_number_mutations);
    //HC_SHMCreator shm_creator(shm_creation_strategy);

    for(auto it = base_repertoire->begin(); it != base_repertoire->end(); it++) {
        cout << "New base antibody, multiplicity: " << it->Multiplicity() << endl;
        for(size_t i = 0; i < it->Multiplicity(); i++) {
            auto variable_region_ptr = it->IgVariableRegion()->Clone();

            //variable_region_ptr = shm_creator.CreateSHM(variable_region_ptr);
            size_t multiplicity = mutated_multiplicity_creator.AssignMultiplicity(variable_region_ptr);
            mutated_repertoire->Add(LC_Cluster(variable_region_ptr, multiplicity));
            cout << "----" << endl;
        }
        cout << "-----------------------------" << endl;
        return mutated_repertoire;
    }
    return mutated_repertoire;
}

int CreateLCRepertoire(LC_InputParams params) {
    auto lc_database = CreateLCDatabase(params);
    LC_Repertoire_Ptr base_repertoire = CreateBaseLCRepertoire(params, lc_database);

    cout << "Base repertoire:" << endl;
    for(auto it = base_repertoire->begin(); it != base_repertoire->end(); it++) {
        cout << it->IgVariableRegion()->VDJ_Recombination()->Sequence() << " " << it->Multiplicity() << endl;
        cout << it->IgVariableRegion()->GetCDRSettings();
    }

    LC_Repertoire_Ptr mutated_repertoire = CreateMutatedLCRepertoire(params, base_repertoire);
    return 0;
}