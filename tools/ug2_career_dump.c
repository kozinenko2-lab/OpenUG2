/* Optional local-data audit. Never commit the output: it contains data
 * derived from the user's legally owned NFS Underground 2 installation. */
#include "ug2_career_file.h"
#include "ug2_career_mod.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
    if(argc!=2 && argc!=3){
        fprintf(stderr,"usage: %s /path/to/GLOBAL/GlobalB.lzc [mods/career.cfg]\n",argv[0]);
        return 2;
    }
    UG2CareerIndex *catalog=(UG2CareerIndex *)calloc(1,sizeof *catalog);
    if(!catalog)return 1;
    int ok=ug2_career_load_file(argv[1],catalog);
    if(ok && argc==3) {
        unsigned changed=0;
        if(!ug2_career_apply_mods_file(catalog,argv[2],&changed)) {
            fprintf(stderr,"mod file invalid or contains unknown race ID: %s\n",argv[2]);
            free(catalog);return 1;
        }
        printf("modded_race_rewards: %u\n",changed);
    }
    if(ok) {
        printf("career_sections: %u\nrace_records: %u\nunique_races: %u\n"
               "duplicate_races: %u\nstages: %u\nsponsors: %u\n",
               catalog->career_sections,catalog->total_race_records,
               catalog->unique_races,catalog->repeated_races,
               catalog->stage_records,catalog->sponsor_records);
        for(unsigned i=0;i<catalog->unique_races;i++){
            const CareerSourceRace *e=&catalog->race[i];
            printf("%u,%s,stage=%u,prize=%u,opponents=%u\n",
                   catalog->section[i],e->id,e->stage,
                   e->cash_value,e->opponents);
        }
    } else fprintf(stderr,"unsupported or corrupted GlobalB career data\n");
    free(catalog);
    return ok?0:1;
}
