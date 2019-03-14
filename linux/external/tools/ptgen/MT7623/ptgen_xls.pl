#!/usr/local/bin/perl -w
use File::Basename;
BEGIN
{
  $LOCAL_PATH = dirname($0);
}
my $CD_ALPS;
$CD_ALPS="$LOCAL_PATH/../../../../../../.."; 
my ${PLATFORM}= $ENV{PLATFORM};
my ${PROJECT}= $ENV{PROJECT};
my $PART_TABLE_FILENAME  = "$CD_ALPS/device/mediatek/build/build/tools/ptgen/${PLATFORM}/partition_table_${PLATFORM}.xls"; # excel file name
my $dir1 = "$CD_ALPS/device";		#Project maynot @ /device/mediatek but /device/companyA.
my @arrayOfFirstLevelDirs;
my $SearchFile = 'partition_table_MT7623.xls'; #Search File Name  
my $found=0;
  
opendir(DIR, $dir1) or die $!;
#Search First Level path of the dir and save dirs in this path to @arrayOfFirstLevelDirs
while (my $file = readdir(DIR)) {
# A file test to check that it is a directory
  next unless (-d "$dir1/$file");
  next unless ( $file !~ m/^\./); #ignore dir prefixed with .
  push @arrayOfFirstLevelDirs, "$dir1/$file";
}
closedir(DIR);
foreach $i (@arrayOfFirstLevelDirs)
{
  #search folder list+{project}/partition_table_MT6572.xls existence
  $PROJECT_PART_TABLE_FILENAME = $i."\/".$PROJECT."\/".$SearchFile;
  if( -e $PROJECT_PART_TABLE_FILENAME)
  {
  	  $found = 1;
      print "$PROJECT_PART_TABLE_FILENAME";
      last;
  }
}
if($found == 0)
{
	print "$PART_TABLE_FILENAME";
}
