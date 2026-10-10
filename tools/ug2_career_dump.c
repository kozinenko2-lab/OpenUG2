/* Optional local-data audit. Never commit the output: it contains data
 * derived from the user's legally owned NFS Underground 2 installation. */
#include "ug2_career_file.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
    if(argc!=2){
        fprintf(stderr,"usage: %s /path/to/GLOBAL/GlobalB.lzc\n",argv[0]);
        return 2;
    }
    UG2CareerIndex *catalog=(UG2CareerIndex *)calloc(1,sizeof *catalog);
    if(!catalog)return 1;
    int ok=ug2_career_load_file(argv[1],catalog);
    if(ok) {
        printf("career_sections: %u\nrace_records: %u\nunique_races: %u\n"
               "duplicate_races: %u\nstages: %u\nsponsors: %u\nshops: %u\n",
               catalog->career_sections,catalog->total_race_records,
               catalog->unique_races,catalog->repeated_races,
               catalog->stage_records,catalog->sponsor_records,
               catalog->shop_records);
        for(unsigned i=0;i<catalog->unique_races;i++){
            const CareerSourceRace *e=&catalog->race[i];
            printf("%u,%s,stage=%u,prize=%u,opponents=%u\n",
                   catalog->section[i],e->id,e->stage,
                   e->cash_value,e->opponents);
        }
        /* Separate category prevents a World's shop trigger from being
         * mistaken for a race TrackID or a completion signal. */
        for(unsigned i=0;i<catalog->shop_records;i++) {
            const UG2CareerShop *shop=&catalog->shops[i];
            printf("shop,section=%u,name=%s,type=%u,stage=%u,hidden=%u,"
                   "movie=%s,file=%s,trigger=%08x,event=%08x,"
                   "event_required=%u\n",
                   (unsigned)shop->section,shop->name,
                   (unsigned)shop->shop_type,(unsigned)shop->stage,
                   (unsigned)shop->initially_hidden,shop->intro_movie,
                   shop->filename,(unsigned)shop->trigger_key,
                   (unsigned)shop->required_event,
                   (unsigned)shop->unlocked_by_event);
        }
    } else fprintf(stderr,"unsupported or corrupted GlobalB career data\n");
    free(catalog);
    return ok?0:1;
}
