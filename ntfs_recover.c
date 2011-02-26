#include "ntfs_recover.h"

int main(int argc, char *argv[])
{
   if(argc < 2)
   {
      printf("Usage: %s <NTFS_fs>\n", argv[0]);
      return -1;
   }
   NTFS_BOOT_SECTOR boot_sector;
   FILE *fp = fopen(argv[1], "rb");
   if(fread(&boot_sector, sizeof(char), sizeof(boot_sector), fp)
         != sizeof(NTFS_BOOT_SECTOR))
   {
      fprintf(stderr, "[ERROR] Reading file failed\n");
      fclose(fp);
      return -1;
   }
   ntfs_volume *vol = (ntfs_volume *)malloc(sizeof(ntfs_volume));
   if(vol == NULL)
   {
      fprintf(stderr, "[ERROR] Allocating memory for NTFS Volumne failed\n");
      return -1;
   }
   fill_ntfs_info(vol, boot_sector);
   load_ntfs_mft(vol);

   free(vol);
   fclose(fp);
   return 0;
}

void load_ntfs_mft(ntfs_volume *vol)
{
}

void fill_ntfs_info(ntfs_volume *vol, NTFS_BOOT_SECTOR s)
{
   BIOS_PARAMETER_BLOCK b = s.bpb;

   vol->sector_size = b.bytes_per_sector;
   vol->cluster_size = b.bytes_per_sector * b.sectors_per_cluster;
   vol->nr_clusters = s.number_of_sectors / b.sectors_per_cluster;
   vol->mft_lcn = s.mft_lcn;
   vol->mftmirr_lcn = s.mftmirr_lcn;
   vol->mft_record_size = s.clusters_per_mft_record * vol->cluster_size;
   vol->indx_record_size = s.clusters_per_index_record * vol->cluster_size;
   vol->mft_zone_start = 0;
   vol->mft_zone_pos = vol->mft_lcn;
   vol->mft_zone_end = vol->mft_lcn + (vol->nr_clusters >> 3); //12.5%
   vol->data1_zone_pos = vol->mft_zone_end;
   vol->data2_zone_pos = 0;
   vol->mft_data_pos = 24; // MFT Record 24

   printf("NTFS BOOT SECTOR INFO\n");
   printf("--------------------------------------------\n");
   printf(" [INFO] Number of Sectors: %lld\n", s.number_of_sectors);
   printf(" [INFO] Cluster location of MFT Data: %lld\n", s.mft_lcn);
   printf(" [INFO] Cluster location of MFT copy: %lld\n", s.mftmirr_lcn);
   printf(" [INFO] Clusters per MFT Record: %d\n", s.clusters_per_mft_record);
   printf(" [INFO] Clusters per Index Record: %d\n", s.clusters_per_index_record);
   printf(" [INFO] Boot Sector Checksum: 0x%08X\n", s.checksum);
   printf("\n");

   printf("BIOS PARAMETER BLOCK INFO\n");
   printf("--------------------------------------------\n");
   printf(" [INFO] Bytes per Sector: %d\n", b.bytes_per_sector);
   printf(" [INFO] Sectors per Cluster: %d\n", b.sectors_per_cluster);
   printf(" [INFO] MFT zone size: %d\n", b.sectors_per_cluster*s.number_of_sectors>>3); //12.5%
   printf(" [INFO] Data1 Zone Position: 0x%08X\n", vol->data1_zone_pos);
   printf(" [INFO] Data2 Zone Position: 0x%08X\n", vol->data2_zone_pos);

   printf("\n");
}

