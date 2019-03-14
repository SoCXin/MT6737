package mtk_modem;
use strict;
use File::Basename;
use Exporter;
use vars qw(@ISA @EXPORT @EXPORT_OK);
@ISA = qw(Exporter);
@EXPORT		= qw(Get_MD_X_From_Bind_Sys_Id Get_MD_YY_From_Image_Type Parse_MD_Struct Check_MD_Info get_modem_file_mapping get_modem_name);
@EXPORT_OK	= qw(Get_MD_X_From_Bind_Sys_Id Get_MD_YY_From_Image_Type Parse_MD_Struct Check_MD_Info get_modem_file_mapping get_modem_name);


# availible yy for specified x, the yy value is not used at this moment
our %MTK_MODEM_MAP_X_TO_YY =
(
	1 => "2g wg tg lwg ltg sglte ultg ulwg ulwtg ulwcg ulwctg",
	2 => "2g wg tg lwg ltg sglte ultg ulwg ulwtg ulwcg ulwctg",
	3 => "2g 3g ulwcg ulwctg",
	5 => "2g wg tg lwg ltg sglte"
);

our %MTK_MODEM_MAP_YY_TO_IMAGE_TYPE =
(
	"2g"	=> 0x01,
	"3g"	=> 0x02,
	"wg"	=> 0x03,
	"tg"	=> 0x04,
	"lwg"	=> 0x05,
	"ltg"	=> 0x06,
	"sglte"	=> 0x07,
	"ultg"	=> 0x08,
	"ulwg"	=> 0x09,
	"ulwtg"	=> 0x0A,
	"ulwcg"	=> 0x0B,
	"ulwctg"	=> 0x0C
);

# input: bind_sys_id
# output: x
sub Get_MD_X_From_Bind_Sys_Id
{
	my $input = shift @_;
	if (exists $MTK_MODEM_MAP_X_TO_YY{$input})
	{
		return $input;
	}
	elsif ($input == 0)
	{
		return 1;
	}
	else
	{
		die "Invalid modem bind_sys_id = $input";
	}
}

# input: image_type
# output: yy
sub Get_MD_YY_From_Image_Type
{
	my $input = shift @_;
	my $output = "";
	foreach my $key (keys %MTK_MODEM_MAP_YY_TO_IMAGE_TYPE)
	{
		if ($MTK_MODEM_MAP_YY_TO_IMAGE_TYPE{$key} == $input)
		{
			$output = $key;
		}
	}
	if ($output eq "")
	{
		die "Invalid modem image_type = $input";
	}
	return $output;
}

# input: aaa/bbb.c
# output: bbb.c
sub Get_Basename
{
	my ($tmpName) = @_;
	my $baseName;
	$baseName = $1 if ($tmpName =~ /.*\/(.*)/);
	return $baseName;
}

# get modem header struct from modem image content
sub Parse_MD_Struct
{
	my ($ref_hash, $parse_md_file) = @_;

	my $buffer;
	my $length1 = 188; # modem image rear
	my $length2 = 172;
	my $length3 = 128; # modem.img project name
	my $length4 = 36;  # modem.img flavor
	my $length5 = 64;  # modem.img verno
	my $whence = 0;
	my $md_file_size = -s $parse_md_file;
	my $pad_size = 0;

	open(MODEM, "< $parse_md_file") or die "Can NOT open file $parse_md_file\n";
	binmode(MODEM);

	seek(MODEM, 0, $whence) or die;
	read(MODEM, $buffer, 4) or die;
	my $magic = unpack("I", $buffer);
	if ($magic == 0x58881688)
	{
		# found IMG_HDR_T
		read(MODEM, $buffer, 4) or die;
		my $dsize = unpack("I", $buffer);
		seek(MODEM, 4 + 4 + 32 + 4 + 4 + 4, $whence) or die;
		read(MODEM, $buffer, 4) or die;
		my $hdr_size = unpack("I", $buffer);
		$pad_size = $md_file_size - $dsize - $hdr_size;
	}

	seek(MODEM, $md_file_size - $pad_size - 4, $whence) or die;
	read(MODEM, $buffer, 4) or die;
	my $size = unpack("L", $buffer);
	if (($size < $length2) or ($size > $md_file_size))
	{
		# sanity test for struct size
	}
	else
	{
		$length1 = $size;
		seek(MODEM, $md_file_size - $pad_size - $length3 - $length4 - $length5 - $length1, $whence) or die "Can NOT seek to the position of modem image rear in \"$parse_md_file\"!\n";
		read(MODEM, $buffer, $length3 + $length4 + $length5 + $length2) or die "Failed to read the rear of the file \"$parse_md_file\"!\n";
		($ref_hash->{"project_name"}, $ref_hash->{"flavor"}, $ref_hash->{"verno"}, $ref_hash->{"check_header"}, $ref_hash->{"header_verno"}, $ref_hash->{"product_ver"}, $ref_hash->{"image_type"}, $ref_hash->{"platform"}, $ref_hash->{"build_time"}, $ref_hash->{"build_ver"}, $ref_hash->{"bind_sys_id"}) = unpack("A128 A36 A64 A12 L L L A16 A64 A64 L", $buffer);
	}

	if ($ref_hash->{'check_header'} ne "CHECK_HEADER")
	{
		#print "!!!" . $ref_hash->{'check_header'} . "!!!\n";
		warn "Reading from MODEM failed! No CHECK_HEADER info!\n";
		delete $ref_hash->{"project_name"};
		delete $ref_hash->{"flavor"};
		delete $ref_hash->{"verno"};
		delete $ref_hash->{"check_header"};
		delete $ref_hash->{"header_verno"};
		delete $ref_hash->{"product_ver"};
		delete $ref_hash->{"image_type"};
		delete $ref_hash->{"platform"};
		delete $ref_hash->{"build_time"};
		delete $ref_hash->{"build_ver"};
		delete $ref_hash->{"bind_sys_id"};
		close(MODEM);
		return 0;
	}
	elsif ($ref_hash->{"header_verno"} < 2)
	{
		delete $ref_hash->{"project_name"};
		delete $ref_hash->{"flavor"};
		delete $ref_hash->{"verno"};
	}
	elsif ($ref_hash->{"header_verno"} >= 3)
	{
		read(MODEM, $buffer, 4*5) or die "Failed to read the rear of the file \"$parse_md_file\"!\n";
		($ref_hash->{"mem_size"}, $ref_hash->{"md_img_size"}, $ref_hash->{"rpc_sec_mem_addr"}, $ref_hash->{"dsp_img_offset"}, $ref_hash->{"dsp_img_size"}) = unpack("L L L L L", $buffer);
	}
	close(MODEM);
	return 1;
}

sub Check_MD_Info
{
	my $ref_hash	= shift @_;
	my $md_id		= shift @_;
	my $MD_IMG		= shift @_;
	my $debug		= shift @_;
#######################################
# Check if MODEM file exists
#######################################
	print "\$MD_IMG = $MD_IMG\n";
	die "[MODEM CHECK FAILED]: The file \"$MD_IMG\" does NOT exist!\n" if (! -e $MD_IMG);

#######################################
# Read mode(2G/3G), debug/release flag, platform info, project info, serial number, etc. from modem.img
#######################################
	my $res = &Parse_MD_Struct($ref_hash, $MD_IMG);
	if ($res != 1)
	{
		die;
	}
	my $MD_IMG_DEBUG = $ref_hash->{"product_ver"};
	my $MD_IMG_MODE = $ref_hash->{"image_type"};
	my $MD_IMG_PLATFORM = $ref_hash->{"platform"};
	my $MD_IMG_PROJECT_ID = $ref_hash->{"build_ver"};
	my $MD_IMG_SERIAL_NO = $ref_hash->{"bind_sys_id"};

#######################################
# Output debug information
#######################################
	if ($debug)
	{
		print "*** Info from $md_id modem image ***\n\n";
		print "modem image is $MD_IMG\n";
		print "\$MD_IMG_DEBUG = $MD_IMG_DEBUG [" . sprintf("0x%08x",$MD_IMG_DEBUG) . "]\n";
		print "\$MD_IMG_MODE = $MD_IMG_MODE [" . sprintf("0x%08x",$MD_IMG_MODE) . "]\n";
		print "\$MD_IMG_PLATFORM = $MD_IMG_PLATFORM\n";
		print "\$MD_IMG_PROJECT_ID = $MD_IMG_PROJECT_ID\n";
		print "\$MD_IMG_SERIAL_NO = $MD_IMG_SERIAL_NO [" . sprintf("0x%08x",$MD_IMG_SERIAL_NO) . "]\n";
	}
	return 0;
}

# get modem image name for _x_yy_z part from modem image content
sub get_modem_suffix
{
	my $ref_header	= shift @_;
	my $flag_force	= shift @_;
	my $naming_string;
	my $MD_HEADER_VERNO		= $ref_header->{"header_verno"};
	my $MD_IMG_SERIAL_NO	= $ref_header->{"bind_sys_id"};
	my $MD_IMG_MODE			= $ref_header->{"image_type"};
	if (($flag_force == 2) || (($flag_force != 1) && ($MD_HEADER_VERNO == 2)))
	{
		# "*_x_yy_z"
		$naming_string = "_" . Get_MD_X_From_Bind_Sys_Id($MD_IMG_SERIAL_NO);
		$naming_string .= "_" . Get_MD_YY_From_Image_Type($MD_IMG_MODE);
		#if ((exists $ref_header->{"mem_size"}) && ($ref_header->{"mem_size"} > 90*1024*1024))
		#{
		#	$naming_string .= "_" . "s";
		#}
		#else
		#{
			$naming_string .= "_" . "n";
		#}
	}
	elsif (($flag_force == 1) || (($flag_force != 2) && ($MD_HEADER_VERNO == 1)))
	{
		if (($MD_IMG_SERIAL_NO == 1) || ($MD_IMG_SERIAL_NO == 0))
		{
			# as is
		}
		elsif ($MD_IMG_SERIAL_NO == 2)
		{
			$naming_string = "_sys2";
		}
		else
		{
			die "Invalid modem bind_sys_id = $MD_IMG_SERIAL_NO";
		}
	}
	else
	{
		die "Invalid modem header_verno = $MD_HEADER_VERNO";
	}
	return $naming_string;
}

sub get_modem_name
{
	my $ref_option	= shift @_;
	my $bind_sys_id	= shift @_;
	my $flag_force;
	my $image_type;
	if (exists $ref_option->{"MTK_MD" . $bind_sys_id . "_SUPPORT"})
	{
		if ($ref_option->{"MTK_MD" . $bind_sys_id . "_SUPPORT"} eq "modem_2g")
		{
			$image_type = 1;
			$flag_force = 1;
		}
		elsif ($ref_option->{"MTK_MD" . $bind_sys_id . "_SUPPORT"} eq "modem_3g")
		{
			$image_type = 2;
			$flag_force = 1;
		}
		else
		{
			$image_type = $ref_option->{"MTK_MD" . $bind_sys_id . "_SUPPORT"};
			$flag_force = 2;
		}
	}
	elsif ($bind_sys_id == 1)
	{
		if ($ref_option->{"MTK_MODEM_SUPPORT"} eq "modem_2g")
		{
			$image_type = 1;
			$flag_force = 1;
		}
		elsif ($ref_option->{"MTK_MODEM_SUPPORT"} eq "modem_3g")
		{
			$image_type = 2;
			$flag_force = 1;
		}
	}
	my %temp_feature = ("bind_sys_id" => $bind_sys_id, "image_type" => $image_type, "header_verno" => $flag_force);
	my $naming_string = "modem" . &get_modem_suffix(\%temp_feature, $flag_force) . ".img";
	return $naming_string;
}

sub set_modem_name
{
	my $ref_hash	= shift @_;
	my $ref_array	= shift @_;
	my $over_method	= shift @_;
	my $over_base	= shift @_;
	my $over_suffix	= shift @_;
	my $over_ext	= shift @_;
	foreach my $file (@$ref_array)
	{
		my $basename;
		my $dirname;
		my $extname;
		($basename, $dirname, $extname) = fileparse($file, qr/\.[^.]*/);
		if ($over_method == 0)
		{
			# add suffix in basename
			$basename = $over_base if ($over_base ne "*");
			$extname = $over_ext if ($over_ext ne "*");
		}
		elsif ($over_method == 1)
		{
			# add suffix in filename
			$basename .= $extname;
			$extname = "";
		}
		elsif ($over_method == 2)
		{
			# no change
			$over_suffix = "";
		}
		$ref_hash->{$file} = $basename . $over_suffix . $extname;
	}
}

sub find_modem_bin
{
	my $ref_hash_filelist	= shift @_;
	my $ref_hash_feature	= shift @_;
	my $path_of_bin			= shift @_;
	my $modem_bin_file;
	my $modem_bin_link;
	$ref_hash_feature->{"TYPE_OF_BIN"} = 0;
	my @wildcard_list_0 = &find_modem_glob($path_of_bin . "*_MDBIN_*.bin");
	if (scalar @wildcard_list_0 == 1)
	{
		$modem_bin_link = $wildcard_list_0[0];
		$modem_bin_file = $modem_bin_link;
	}
	else
	{
		my @wildcard_list_1 = &find_modem_glob($path_of_bin . "*_PCB01_*.bin");
		if (scalar @wildcard_list_1 == 1)
		{
			my $wildcard_file = $wildcard_list_1[0];
			if ((! -d $wildcard_file) && (-e $wildcard_file))
			{
				$modem_bin_link = $wildcard_file;
				$ref_hash_feature->{"TYPE_OF_BIN"} |= 0b00000001;
			}
			if ((-d $wildcard_file) && (-e $wildcard_file . "/ROM"))
			{
				$modem_bin_link = $wildcard_file . "/ROM";
				$ref_hash_feature->{"TYPE_OF_BIN"} |= 0b00000010;
			}
		}
		elsif (scalar @wildcard_list_1 == 0)
		{
			if (-e $path_of_bin . "modem_sys2.img")
			{
				$modem_bin_link = $path_of_bin . "modem_sys2.img";
			}
			if (-e $path_of_bin . "modem.img")
			{
				$modem_bin_link = $path_of_bin . "modem.img";
			}
			if (-e $path_of_bin . "cp.rom")
			{
				$modem_bin_link = $path_of_bin . "cp.rom";
			}
		}
		else
		{
			print "[ERROR] More than one modem images are found: " . join(" ", @wildcard_list_1) . "\n";
		}
		if (-e $path_of_bin . "modem.img")
		{
			$modem_bin_file = $path_of_bin . "modem.img";
			$ref_hash_feature->{"TYPE_OF_BIN"} |= 0b00000100;
		}
		else
		{
			$modem_bin_file = $modem_bin_link;
		}
	}
	return ($modem_bin_file, $modem_bin_link);
}

sub find_modem_mak
{
	my $ref_hash_filelist	= shift @_;
	my $ref_hash_feature	= shift @_;
	my $path_of_mak			= shift @_;
	my $modem_mak_pattern = "*.mak";
	my $modem_mak_file;
	my @wildcard_list = &find_modem_glob($path_of_mak . $modem_mak_pattern);
	foreach my $wildcard_file (@wildcard_list)
	{
		if ($wildcard_file =~ /\bMMI_DRV_DEFS\.mak/)
		{
		}
		elsif ($wildcard_file =~ /~/)
		{
			$modem_mak_file = $wildcard_file;
		}
		elsif ($modem_mak_file eq "")
		{
			$modem_mak_file = $wildcard_file;
		}
		else
		{
			print "[WARNING] Unknown project makefile: " . $wildcard_file . " or " . $modem_mak_file . "\n";
		}
	}
	return $modem_mak_file;
}

sub get_modem_file_mapping
{
	my $ref_hash_filelist	= shift @_;
	my $ref_hash_feature	= shift @_;
	my $ref_hash_option		= shift @_;
	my $project_root		= shift @_;
	my $type_of_load		= shift @_;
	my $branch_of_ap		= shift @_;

	my $path_of_bin;
	my $path_of_database;
	my $modem_bin_file;
	my $modem_bin_link;
	my $modem_bin_prefix;
	my $modem_bin_suffix;
	my $modem_mak_file;
	my @checklist_of_general;
	my @checklist_of_dsp_bin;
	my @checklist_of_append;
	my @checklist_of_remain;

	my $rule_of_rename = 2;
	if (($branch_of_ap eq "") or ($branch_of_ap =~ /KK/))
	{
		$rule_of_rename = 2;
	}
	elsif ($branch_of_ap =~ /ALPS\.(JB|JB2)\./)
	{
		$rule_of_rename = 1;
	}
	elsif ($branch_of_ap =~ /ALPS\.(ICS|GB)\d*\./)
	{
		$rule_of_rename = 1;
	}
	else
	{
		$rule_of_rename = 2;
	}

	$project_root =~ s/\/$//;
	if ($type_of_load == 0)
	{
		$path_of_bin = $project_root . "/" . "bin/";
	}
	else
	{
		$path_of_bin = $project_root . "/";
	}
	($modem_bin_file, $modem_bin_link) = &find_modem_bin($ref_hash_filelist, $ref_hash_feature, $path_of_bin);
	$modem_mak_file = &find_modem_mak($ref_hash_filelist, $ref_hash_feature, $path_of_bin);
	if (($modem_bin_file eq "") or ($modem_bin_link eq ""))
	{
		die "The modem bin is not found";
	}
	elsif (! -e $modem_bin_file)
	{
		die "The file " . $modem_bin_file . " does NOT exist!";
	}
	elsif ($ref_hash_feature->{"TYPE_OF_BIN"} == 0b00000100)
	{
		# BACH
		$modem_bin_suffix = "";
		# hard code for PMS
		$ref_hash_feature->{"bind_sys_id"} = 1;
		if ($ref_hash_option)
		{
			$ref_hash_option->{"PURE_AP_USE_EXTERNAL_MODEM"} = "yes";
			if ($modem_bin_link =~ /modem_sys2\.img/)
			{
				$ref_hash_option->{"MT6280_SUPER_DONGLE"} = "no";
			}
			else
			{
				$ref_hash_option->{"MT6280_SUPER_DONGLE"} = "yes";
			}
		}
	}
	else
	{
		my $res = &Parse_MD_Struct($ref_hash_feature, $modem_bin_file);
		if ($res == 1)
		{
			$modem_bin_prefix = "md" . $ref_hash_feature->{"bind_sys_id"};
			$modem_bin_suffix = &get_modem_suffix($ref_hash_feature, $rule_of_rename);
			$ref_hash_feature->{"suffix"} = $modem_bin_suffix;
			print "Modem bin file is " . $modem_bin_file . "\n";
			if ($ref_hash_option)
			{
				$ref_hash_option->{"MTK_ENABLE_MD" . $ref_hash_feature->{"bind_sys_id"}} = "yes";
				$ref_hash_option->{"MTK_MD" . $ref_hash_feature->{"bind_sys_id"} . "_SUPPORT"} = $ref_hash_feature->{"image_type"};
			}
		}
		else
		{
			$ref_hash_feature->{"TYPE_OF_BIN"} = 0b00001000;
		}
	}
	if ($ref_hash_filelist)
	{
		if ($ref_hash_feature->{"TYPE_OF_BIN"} == 0b00000100)
		{
			# BACH
			push(@checklist_of_remain, $path_of_bin . "modem*.database");
			push(@checklist_of_remain, $path_of_bin . "boot.img");
			push(@checklist_of_remain, $path_of_bin . "configpack.bin");
			push(@checklist_of_remain, $path_of_bin . "fc-*.bin");
			push(@checklist_of_remain, $path_of_bin . "ful.bin");
			push(@checklist_of_remain, $path_of_bin . "md_bl.bin");
			push(@checklist_of_remain, $path_of_bin . "modem*.img");
			push(@checklist_of_remain, $path_of_bin . "preloader.bin");
			push(@checklist_of_remain, $path_of_bin . "SECURE_RO");
			push(@checklist_of_remain, $path_of_bin . "system.img");
			push(@checklist_of_remain, $path_of_bin . "uboot.bin");
			push(@checklist_of_remain, $path_of_bin . "userdata.img");
			push(@checklist_of_remain, $path_of_bin . "*.cfg");
			if (-e $path_of_bin . "catcher_filter.bin")
			{
				&set_modem_name($ref_hash_filelist, [$path_of_bin . "catcher_filter.bin"], 0, "*", "_ext", "*");
			}
			else
			{
				push(@checklist_of_remain, $path_of_bin . "catcher_filter_ext.bin");
			}
		}
		elsif ($ref_hash_feature->{"TYPE_OF_BIN"} == 0b00001000)
		{
			# MAUI DSDA
			&set_modem_name($ref_hash_filelist, [$modem_bin_file], 0, "modem", $modem_bin_suffix, ".img");
			if ($type_of_load == 2)
			{
				$path_of_database = $project_root . "/" . "../../" . "tst/database_classb/";
			}
			else
			{
				$path_of_database = $project_root . "/";
			}
			push(@checklist_of_remain, $path_of_database . "BPLGUInfoCustom*");
			my @wildcard_list = &find_modem_cfg(\@checklist_of_remain, dirname($modem_bin_link) . "/");
			if (scalar @wildcard_list == 1)
			{
				&set_modem_name($ref_hash_filelist, \@wildcard_list, 0, "EXT_MODEM_BB", "", "*");
			}
			else
			{
				die scalar @wildcard_list . " unexpected cfg: " . join(" ", @wildcard_list);
			}
			if (-e $path_of_database . "catcher_filter.bin")
			{
				&set_modem_name($ref_hash_filelist, [$path_of_database . "catcher_filter.bin"], 0, "*", "_ext", "*");
			}
			else
			{
				push(@checklist_of_remain, $path_of_bin . "catcher_filter_ext.bin");
			}
		}
		elsif (($ref_hash_feature->{"image_type"} >= 0x08) and ($ref_hash_feature->{"image_type"} <= 0x0C))
		{
			# UMOLY
			if (1)
			{
				my $filename = basename($modem_bin_file);
				&set_modem_name($ref_hash_filelist, [$path_of_bin . "./" . $filename], 0, "modem", $modem_bin_suffix, ".img");
				&set_modem_name($ref_hash_filelist, [$modem_bin_file], 0, $modem_bin_prefix . "rom", "", ".img");
			}
			if (1)
			{
				my @wildcard_list = &find_modem_glob($path_of_bin . "*DSP*.bin");
				if (scalar @wildcard_list != 1)
				{
					die scalar @wildcard_list . " unexpected dsp bin: " . join(" ", @wildcard_list);
				}
				my $filename = basename($wildcard_list[0]);
				&set_modem_name($ref_hash_filelist, [$path_of_bin . "./" . $filename], 0, "dsp", $modem_bin_suffix, "*");
				&set_modem_name($ref_hash_filelist, [$path_of_bin . $filename], 0, $modem_bin_prefix . "dsp", "", ".img");
			}
			if (-e $path_of_bin . "md1arm7.img")
			{
				&set_modem_name($ref_hash_filelist, [$path_of_bin . "./" . "md1arm7.img"], 0, "armv7", $modem_bin_suffix, ".bin");
				&set_modem_name($ref_hash_filelist, [$path_of_bin . "md1arm7.img"], 0, $modem_bin_prefix . "arm7", "", ".img");
			}
			if ($type_of_load == 0)
			{
				$path_of_database = $project_root . "/" . "dhl/database/";
			}
			else
			{
				$path_of_database = $project_root . "/";
			}
			foreach my $file (&find_modem_glob($path_of_database . "MDDB*.EDB"))
			{
				if ($file =~ /MDDB\.[LP]_/)
				{
				}
				else
				{
					push(@checklist_of_general, $file);
				}
			}
			foreach my $file (&find_modem_glob($path_of_database . "DbgInfo_*"))
			{
				if ($file =~ /DbgInfo_[LP]_/)
				{
				}
				else
				{
					push(@checklist_of_append, $file);
				}
			}
			push(@checklist_of_general, $path_of_database . "catcher_filter.bin") if (-e $path_of_database . "catcher_filter.bin");
			push(@checklist_of_general, $path_of_database . "em_filter.bin") if (-e $path_of_database . "em_filter.bin");
			push(@checklist_of_general, $path_of_database . "mdm_layout_desc.dat") if (-e $path_of_database . "mdm_layout_desc.dat");
		}
		elsif (($ref_hash_feature->{"image_type"} >= 1) and ($ref_hash_feature->{"image_type"} <= 7))
		{
			# MOLY or C2K
			if ($ref_hash_feature->{"bind_sys_id"} == 3)
			{
				my $filename = basename($modem_bin_file);
				&set_modem_name($ref_hash_filelist, [$path_of_bin . "./" . $filename], 0, "modem", $modem_bin_suffix, ".img");
				&set_modem_name($ref_hash_filelist, [$modem_bin_file], 0, $modem_bin_prefix . "rom", "", ".img");
			}
			else
			{
				&set_modem_name($ref_hash_filelist, [$modem_bin_file], 0, "modem", $modem_bin_suffix, ".img");
			}
			if ($type_of_load == 0)
			{
				if (-d $project_root . "/" . "dhl/database")
				{
					$path_of_database = $project_root . "/" . "dhl/database/";
				}
				elsif (-d $project_root . "/" . "tst/database/")
				{
					$path_of_database = $project_root . "/" . "tst/database/";
				}
				else
				{
					$path_of_database = $project_root . "/" . "bin/";
				}
			}
			else
			{
				$path_of_database = $project_root . "/";
			}
			push(@checklist_of_append, dirname($modem_bin_link) . "/" . "SECURE_RO") if (-e dirname($modem_bin_link) . "/" . "SECURE_RO");
			push(@checklist_of_append, $path_of_database . "BPLGUInfoCustom*");
			if (1)
			{
				my @wildcard_list = &find_modem_glob($path_of_database . "MDDB_*");
				foreach my $file (@wildcard_list)
				{
					if ($file =~ /\.EDB$/)
					{
						push(@checklist_of_general, $file);
					}
					else
					{
						push(@checklist_of_append, $file);
					}
				}
			}
			push(@checklist_of_append, $path_of_database . "BPMdMetaDatabase_*");
			push(@checklist_of_append, $path_of_database . "DbgInfo*");
			push(@checklist_of_general, $path_of_database . "catcher_filter.bin") if (-e $path_of_database . "catcher_filter.bin");
			push(@checklist_of_general, $path_of_database . "em_filter.bin") if (-e $path_of_database . "em_filter.bin");
			push(@checklist_of_general, $path_of_database . "mdm_layout_desc.dat") if (-e $path_of_database . "mdm_layout_desc.dat");
			push(@checklist_of_general, $path_of_database . "mcddll.dll") if (-e $path_of_database . "mcddll.dll");
			# MT6575/MT6577
			push(@checklist_of_general, dirname($modem_bin_link) . "/" . "DSP_BL") if (-e dirname($modem_bin_link) . "/" . "DSP_BL");
			push(@checklist_of_general, dirname($modem_bin_link) . "/" . "DSP_ROM") if (-e dirname($modem_bin_link) . "/" . "DSP_ROM");
			# MT6582/MT6592/MT6595
			push(@checklist_of_dsp_bin, $path_of_bin . "*DSP*.bin") if (&find_modem_glob($path_of_bin . "*DSP*.bin"));
			foreach my $file (@checklist_of_dsp_bin)
			{
				my @wildcard_list = &find_modem_glob($file);
				if (scalar @wildcard_list == 1)
				{
					&set_modem_name($ref_hash_filelist, \@wildcard_list, 0, "dsp", $modem_bin_suffix, "*");
				}
				else
				{
					die scalar @wildcard_list . " unexpected dsp bin: " . join(" ", @wildcard_list);
				}
			}
			# C2K
			my $path_of_fsm;
			my $path_of_rom;
			my $path_of_obj = $path_of_bin;
			$path_of_obj =~ s/\/build\/.*$/\/obj\//i;
			if ($type_of_load == 2)
			{
				$path_of_fsm = $path_of_bin . "fsm_files/images/";
				$path_of_rom = $path_of_bin . "lib/";
				$path_of_rom =~ s/\/build\//\/mtk_rel\//i;
			}
			else
			{
				$path_of_fsm = $path_of_bin;
				$path_of_rom = $path_of_bin;
			}
			if (-e $path_of_rom . "boot.rom")
			{
				push(@checklist_of_general, $path_of_rom . "boot.rom");
			}
			else
			{
				my @wildcard_list = &find_modem_glob($path_of_rom . "*_BOOT_*.bin");
				&set_modem_name($ref_hash_filelist, \@wildcard_list, 0, "boot", $modem_bin_suffix, "rom");
			}
			push(@checklist_of_general, $path_of_fsm . "fsm_rf_df.img") if (-e $path_of_fsm . "fsm_rf_df.img");
			push(@checklist_of_general, $path_of_fsm . "fsm_rw_df.img") if (-e $path_of_fsm . "fsm_rw_df.img");
			push(@checklist_of_general, $path_of_fsm . "fsm_cust_df.img") if (-e $path_of_fsm . "fsm_cust_df.img");
			if (-e $path_of_bin . "catcher_filter.bin")
			{
				push(@checklist_of_general, $path_of_bin . "catcher_filter.bin");
			}
			elsif (-e $path_of_bin . "bin/catcher_filter.bin")
			{
				push(@checklist_of_general, $path_of_bin . "bin/catcher_filter.bin");
			}
			elsif (-e $path_of_obj . "catcher_filter.bin")
			{
				push(@checklist_of_general, $path_of_obj . "catcher_filter.bin");
			}
		}
		else
		{
			die "Unknown modem spec";
		}
		foreach my $file (@checklist_of_general)
		{
			my @wildcard_list = &find_modem_glob($file);
			&set_modem_name($ref_hash_filelist, \@wildcard_list, 0, "*", $modem_bin_suffix, "*");
		}
		foreach my $file (@checklist_of_append)
		{
			my @wildcard_list = &find_modem_glob($file);
			&set_modem_name($ref_hash_filelist, \@wildcard_list, 1, "*", $modem_bin_suffix, "*");
		}
		foreach my $file (@checklist_of_remain)
		{
			my @wildcard_list = &find_modem_glob($file);
			&set_modem_name($ref_hash_filelist, \@wildcard_list, 2, "*", "", "*");
		}
		if ($modem_mak_file ne "")
		{
			my @wildcard_list = &find_modem_glob($modem_mak_file);
			&set_modem_name($ref_hash_filelist, \@wildcard_list, 0, "modem", $modem_bin_suffix, "*");
		}
		# AOSP
		$ref_hash_feature->{"Android.mk"} = &gen_android_mk();
	}
}

sub find_modem_cfg
{
	my $ref_hash_filelist	= shift @_;
	my $path_of_bin			= shift @_;
	my $READ_HANDLE;
	my @files = &find_modem_glob($path_of_bin . "*.cfg");
	foreach my $file (@files)
	{
		if (open($READ_HANDLE, "<$file"))
		{
			my @lines = <$READ_HANDLE>;
			close $READ_HANDLE;
			foreach my $line (@lines)
			{
				if ($line =~ /^\s*-\s*file:\s*(\S+)/)
				{
					push(@$ref_hash_filelist, $path_of_bin . $1) if ($ref_hash_filelist);
				}
			}
		}
	}
	return @files;
}

sub find_modem_glob
{
	my @dirs = @_;
	my @result;
	foreach my $dir (@dirs)
	{
		$dir =~ s/([\[\]\(\)\{\}\^])/\\$1/g;
		push(@result, glob $dir);
	}
	return @result;
}

sub gen_android_mk
{
	my $text = '
LOCAL_PATH := $(call my-dir)
MTK_MODEM_LOCAL_PATH := $(LOCAL_PATH)
MTK_MODEM_MDDB_FILES :=
MTK_MODEM_FIRMWARE_FILES :=
MTK_MODEM_EXTMDDB_FILES :=
MTK_MODEM_PARTITION_FILES :=
MTK_MODEM_MAP_X_1_TO_YY := 2g wg tg lwg ltg sglte ultg ulwg ulwtg ulwcg ulwctg
MTK_MODEM_MAP_X_2_TO_YY := 2g wg tg lwg ltg sglte ultg ulwg ulwtg ulwcg ulwctg
MTK_MODEM_MAP_X_3_TO_YY := 2g 3g ulwcg ulwctg
MTK_MODEM_MAP_X_5_TO_YY := lwg ltg sglte

##### INSTALL MODEM FIRMWARE #####
$(foreach x,1 2,\
  $(if $(filter yes,$(strip $(MTK_ENABLE_MD$(x)))),\
    $(foreach yy,$(MTK_MODEM_MAP_X_$(x)_TO_YY),\
      $(foreach z,n,\
        $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/modem_$(x)_$(yy)_$(z).img),\
          $(eval MTK_MODEM_FIRMWARE_FILES += modem_$(x)_$(yy)_$(z).img)\
          $(if $(filter l%g sglte ul%g,$(yy)),\
            $(eval MTK_MODEM_FIRMWARE_FILES += dsp_$(x)_$(yy)_$(z).bin)\
          )\
          $(if $(filter yes,$(strip $(MTK_MDLOGGER_SUPPORT))),\
            $(eval MTK_MODEM_FIRMWARE_FILES += catcher_filter_$(x)_$(yy)_$(z).bin)\
          )\
          $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/em_filter_$(x)_$(yy)_$(z).bin),\
            $(eval MTK_MODEM_FIRMWARE_FILES += em_filter_$(x)_$(yy)_$(z).bin)\
          )\
          $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/armv7_$(x)_$(yy)_$(z).bin),\
            $(eval MTK_MODEM_FIRMWARE_FILES += armv7_$(x)_$(yy)_$(z).bin)\
          )\
        )\
      )\
    )\
  )\
)
$(foreach x,3,\
  $(if $(filter yes,$(strip $(MTK_ENABLE_MD$(x)))),\
    $(foreach yy,$(MTK_MODEM_MAP_X_$(x)_TO_YY),\
      $(foreach z,n,\
        $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/modem_$(x)_$(yy)_$(z).img),\
          $(eval MTK_MODEM_FIRMWARE_FILES += modem_$(x)_$(yy)_$(z).img)\
        )\
        $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/boot_$(x)_$(yy)_$(z).rom),\
          $(eval MTK_MODEM_FIRMWARE_FILES += boot_$(x)_$(yy)_$(z).rom)\
        )\
        $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/fsm_rf_df_$(x)_$(yy)_$(z).img),\
          $(eval MTK_MODEM_FIRMWARE_FILES += fsm_rf_df_$(x)_$(yy)_$(z).img)\
        )\
        $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/fsm_rw_df_$(x)_$(yy)_$(z).img),\
          $(eval MTK_MODEM_FIRMWARE_FILES += fsm_rw_df_$(x)_$(yy)_$(z).img)\
        )\
        $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/fsm_cust_df_$(x)_$(yy)_$(z).img),\
          $(eval MTK_MODEM_FIRMWARE_FILES += fsm_cust_df_$(x)_$(yy)_$(z).img)\
        )\
        $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/catcher_filter_$(x)_$(yy)_$(z).bin),\
          $(eval MTK_MODEM_FIRMWARE_FILES += catcher_filter_$(x)_$(yy)_$(z).bin)\
        )\
      )\
    )\
  )\
)
$(foreach x,5,\
  $(if $(filter yes,$(strip $(MTK_ENABLE_MD$(x)))),\
    $(foreach yy,$(MTK_MODEM_MAP_X_$(x)_TO_YY),\
      $(foreach z,n,\
        $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/modem_$(x)_$(yy)_$(z).img),\
          $(eval MTK_MODEM_FIRMWARE_FILES += modem_$(x)_$(yy)_$(z).img)\
          $(eval MTK_MODEM_FIRMWARE_FILES += dsp_$(x)_$(yy)_$(z).bin)\
          $(if $(filter yes,$(strip $(MTK_MDLOGGER_SUPPORT))),\
            $(eval MTK_MODEM_FIRMWARE_FILES += catcher_filter_$(x)_$(yy)_$(z).bin)\
          )\
        )\
      )\
    )\
  )\
)
########INSTALL MODEM DATABASE########
ifeq ($(strip $(MTK_INCLUDE_MODEM_DB_IN_IMAGE)), yes)
ifeq ($(filter generic banyan banyan_x86,$(TARGET_DEVICE)),)
$(foreach x,1 2 3 5,\
  $(if $(filter yes,$(strip $(MTK_ENABLE_MD$(x)))),\
    $(foreach yy,$(MTK_MODEM_MAP_X_$(x)_TO_YY),\
      $(eval MTK_MODEM_DATABASE_FROM := $(wildcard $(MTK_MODEM_LOCAL_PATH)/BPLGUInfoCustomAppSrcP_*_$(x)_$(yy)_*))\
      $(if $(strip $(MTK_MODEM_DATABASE_FROM)),,\
        $(eval MTK_MODEM_DATABASE_FROM := $(wildcard $(MTK_MODEM_LOCAL_PATH)/BPLGUInfoCustomApp_*_$(x)_$(yy)_*))\
      )\
      $(eval MTK_MODEM_MDDB_FILES += $(notdir $(MTK_MODEM_DATABASE_FROM)))\
      $(eval MTK_MODEM_MDDB_FILES += $(notdir $(wildcard $(MTK_MODEM_LOCAL_PATH)/DbgInfo_*_$(x)_$(yy)_*)))\
      $(eval MTK_MODEM_MDDB_FILES += $(notdir $(wildcard $(MTK_MODEM_LOCAL_PATH)/MDDB*_$(x)_$(yy)_*)))\
      $(eval MTK_MODEM_MDDB_FILES += $(notdir $(wildcard $(MTK_MODEM_LOCAL_PATH)/BPMdMetaDatabase_*_$(x)_$(yy)_*)))\
      $(eval MTK_MODEM_MDDB_FILES += $(notdir $(wildcard $(MTK_MODEM_LOCAL_PATH)/mdm_layout_desc_$(x)_$(yy)_*)))\
    )\
  )\
)
endif
endif
########INSTALL MODEM DSDA########
ifneq ($(strip $(MTK_EXTERNAL_MODEM_SLOT)),)
ifneq ($(strip $(MTK_EXTERNAL_MODEM_SLOT)), 0)
ifeq ($(strip $(MTK_DT_SUPPORT)), yes)
ifeq ($(strip $(MTK_MODEM_FIRMWARE_FILES)),)
ifneq ($(wildcard $(MTK_MODEM_LOCAL_PATH)/EXT_MODEM_BB.cfg),)
  MTK_MODEM_FIRMWARE_FILES += \
                              catcher_filter_ext.bin \
                              EXT_MODEM_BB.cfg \
                              $(shell cat $(MTK_MODEM_LOCAL_PATH)/EXT_MODEM_BB.cfg | grep " - file:" | sed -e \'s/ - file://\')
  MTK_MODEM_EXTMDDB_FILES += $(notdir $(wildcard $(MTK_MODEM_LOCAL_PATH)/BPLGUInfoCustomApp*))
endif
endif
endif
endif
endif
########INSTALL MODEM PARTITION########
MTK_MODEM_PARTITION_FILES += $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/md1rom.img),md1rom.img)
MTK_MODEM_PARTITION_FILES += $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/md1dsp.img),md1dsp.img)
MTK_MODEM_PARTITION_FILES += $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/md1arm7.img),md1arm7.img)
MTK_MODEM_PARTITION_FILES += $(if $(wildcard $(MTK_MODEM_LOCAL_PATH)/md3rom.img),md3rom.img)

$(foreach item,$(MTK_MODEM_FIRMWARE_FILES),$(eval $(call mtk-install-modem,$(item),$(TARGET_OUT_ETC)/firmware)))
$(foreach item,$(MTK_MODEM_MDDB_FILES),$(eval $(call mtk-install-modem,$(item),$(TARGET_OUT_ETC)/mddb)))
$(foreach item,$(MTK_MODEM_EXTMDDB_FILES),$(eval $(call mtk-install-modem,$(item),$(TARGET_OUT_ETC)/extmddb)))
$(foreach item,$(MTK_MODEM_PARTITION_FILES),$(eval $(call mtk-install-modem,$(item),$(PRODUCT_OUT))))
MTK_MODEM_DATABASE_FILES := $(MTK_MODEM_INSTALLED_MODULES)
';
	return $text;
}

return 1;

