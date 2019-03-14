#!/usr/bin/perl
use Getopt::Long;
use XML::DOM;

my $relPackageName= "";
my $company= "";
my $project= "";
my $baseProject = "";

&GetOptions(
    "rel=s" => \$relPackageName,
    "c=s" => \$company,
    "p=s" => \$project,
    "base=s" => \$baseProject,
);

if($relPackageName =~ /^\s*$/){
    print "release package is empty $relPackageName\n";
    &usage();
}


my $dom_parser = new XML::DOM::Parser;
my @relPackageNames=split /,/,$relPackageName;
my @relPackages = map { "device/mediatek/build/config/common/releasepackage/" . $_ . ".xml"} @relPackageNames;
my $doc;
my $platform;
my $nodes;
my $n;
my %configList = ();
my %dirList = ();
my %fileList = ();
my %frameworkList = ();
my %androidList = ();
my %appList = ();
my %kernelList = ();
my $key;
   $is = 1;


sub dirName {
    my $path = shift;

    if ($path =~ m#^(.*)/$#) {
        return $1;
    } else {
        return $path;
    }
}

foreach $xmlfile (@relPackages) {
    $doc = $dom_parser->parsefile($xmlfile);

    if ($is == 1) {
        $platform = "";
        my $node = $doc->getElementsByTagName("ReleasePackageName")->item(0)->getChildNodes()->item(0);
        $platform = $node->getNodeValue if (defined($node));
        $is = 0;
    }

    my $configlists = $doc->getElementsByTagName("ConfigList");
    my $nc = $configlists->getLength;
    if ($nc > 0) {
        $nodes = $doc->getElementsByTagName("ConfigList")->item(0)->getElementsByTagName("Config");
        $n = $nodes->getLength;
        for (my $i = 0; $i < $n; $i++) {
            my $node = $nodes->item ($i)->getElementsByTagName("File")->item(0);
            my $fnode = $node->getChildNodes()->item(0) if (defined($node));
            my $fvalue = $fnode->getNodeValue if (defined($fnode));
            $node = $nodes->item ($i)->getElementsByTagName("Old")->item(0);
            my $onode = $node->getChildNodes()->item(0) if (defined($node));
            my $ovalue = $onode->getNodeValue if (defined($onode));
            $node = $nodes->item ($i)->getElementsByTagName("New")->item(0);
            my $nnode = $node->getChildNodes()->item(0) if (defined($node));
            my $nvalue = $nnode->getNodeValue if (defined($nnode));
            next if ($fvalue eq "" or $ovalue eq "");
            $fvalue = dirName("$fvalue");

            $configList{$fvalue}{$ovalue} = $nvalue;
        }
    }

    $nodes = $doc->getElementsByTagName("DirList")->item(0)->getElementsByTagName("ReleaseDirList")->item(0)->getElementsByTagName("Dir");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $dirList{$value} = "Release" if (defined($value) && (!defined($dirList{$value}) || $dirList{$value} eq "UnRelease" || $dirList{$value} eq "Remove"));
    }

    $nodes = $doc->getElementsByTagName("DirList")->item(0)->getElementsByTagName("UnReleaseDirList")->item(0)->getElementsByTagName("Dir");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $dirList{$value} = "UnRelease" if (defined($value) && (!defined($dirList{$value}) ||  $dirList{$value} eq "Remove"));
    }

    $nodes = $doc->getElementsByTagName("DirList")->item(0)->getElementsByTagName("RemovalDirList")->item(0)->getElementsByTagName("Dir");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $dirList{$value} = "Remove" if (defined($value) && !defined($dirList{$value}));
    }

    $nodes = $doc->getElementsByTagName("FileList")->item(0)->getElementsByTagName("ReleaseFileList")->item(0)->getElementsByTagName("File");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $fileList{$value} = "Release" if (defined($value) && (!defined($fileList{$value}) || $fileList{$value} eq "UnRelease"));
    }

    $nodes = $doc->getElementsByTagName("FileList")->item(0)->getElementsByTagName("UnReleaseFileList")->item(0)->getElementsByTagName("File");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $fileList{$value} = "UnRelease" if (defined($value) && !defined($fileList{$value}));
    }

    $nodes = $doc->getElementsByTagName("FrameworkRelease")->item(0)->getElementsByTagName("SourceList")->item(0)->getElementsByTagName("Source");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $frameworkList{$value} = "Source" if (defined($value) && (!defined($frameworkList{$value}) || $frameworkList{$value} eq "Binary"));
    }

    $nodes = $doc->getElementsByTagName("FrameworkRelease")->item(0)->getElementsByTagName("BINList")->item(0)->getElementsByTagName("Binary");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $frameworkList{$value} = "Binary" if (defined($value) && !defined($frameworkList{$value}));
    }

    $nodes = $doc->getElementsByTagName("FrameworkRelease")->item(0)->getElementsByTagName("PartialSourceList")->item(0)->getElementsByTagName("PartialSource");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $base = $nodes->item($i)->getAttribute("base");
        my $module = $nodes->item($i)->getAttribute("module");
        my $_bnodes = $nodes->item($i)->getElementsByTagName("Binary");
        my $_bn = $_bnodes->getLength;
        my $_snodes = $nodes->item($i)->getElementsByTagName("Source");
        my $_sn = $_snodes->getLength;
        my %temp = ();
        my %hashTemp = ();
        my $value = "";
        if (defined($base) and defined($module)) {
            if (defined($frameworkList{"$base#$module"})) {
                my @_temp = split(':', $frameworkList{"$base#$module"});
                %hashTemp = map { $_ => 1 } @_temp;
            }
            for (my $j = 0; $j < $_bn; $j++) {
                my $node = $_bnodes->item ($j)->getChildNodes()->item(0);
                if (defined($node)){
                    my $key = $node->getNodeValue;
                    next if ($key eq "");
                    $hashTemp{$key} = 1;
                }
            }
            for (my $j = 0; $j < $_sn; $j++) {
                my $node = $_snodes->item ($j)->getChildNodes()->item(0);
                if (defined($node)){
                    my $key = $node->getNodeValue;
                    next if ($key eq "");
                    $hashTemp{$key} = 0;
                }
            }
            @temp = sort keys %hashTemp;
            foreach my $data (@temp) {
                if  ($hashTemp{$data} == 1) {
                    if ($value eq "")  {
                        $value = $data;
                    } else {
                        $value = $value . ":" . $data;
                    }
                }
            }
            $frameworkList{"$base#$module"} = $value;
        }
    }

    $nodes = $doc->getElementsByTagName("AndroidRelease")->item(0)->getElementsByTagName("SourceList")->item(0)->getElementsByTagName("Source");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $androidList{$value} = "Source" if (defined($value) && (!defined($androidList{$value}) || $androidList{$value} eq "Binary"));
    }

    $nodes = $doc->getElementsByTagName("AndroidRelease")->item(0)->getElementsByTagName("BINList")->item(0)->getElementsByTagName("Binary");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $androidList{$value} = "Binary" if (defined($value) && !defined($androidList{$base}));
    }

    $nodes = $doc->getElementsByTagName("APPRelease")->item(0)->getElementsByTagName("SourceList")->item(0)->getElementsByTagName("Source");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $appList{$value} = "Source" if (defined($value) && (!defined($appList{$value}) || $appList{$value} eq "Binary"));
    }

    $nodes = $doc->getElementsByTagName("APPRelease")->item(0)->getElementsByTagName("BINList")->item(0)->getElementsByTagName("Binary");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $appList{$value} = "Binary" if (defined($value) && !defined($appList{$base}));
    }

    $nodes = $doc->getElementsByTagName("KernelRelease")->item(0)->getElementsByTagName("SourceList")->item(0)->getElementsByTagName("Source");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $kernelList{$value} = "Source" if (defined($value) && (!defined($kernelList{$value}) || $kernelList{$value} eq "Binary"));
    }

    $nodes = $doc->getElementsByTagName("KernelRelease")->item(0)->getElementsByTagName("BINList")->item(0)->getElementsByTagName("Binary");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        $kernelList{$value} = "Binary" if (defined($value) && !defined($kernelList{$base}));
    }

    $doc->dispose;
}

print "<ReleasePackage>\n";

 print "\t<ReleasePackageName>";
 print "$platform";
 print "</ReleasePackageName>\n";

 print "\t<ConfigList>\n";
   foreach $key (sort keys %configList) {
     foreach $oldConfig (sort keys %{$configList{$key}}){
       print "\t\t<Config>\n";
         print "\t\t\t<File>$key</File>\n";
         print "\t\t\t<Old>$oldConfig</Old>\n";
         print "\t\t\t<New>$configList{$key}{$oldConfig}</New>\n";
       print "\t\t</Config>\n";
     }
   }
 print "\t</ConfigList>\n";

 print "\t<DirList>\n";
  print "\t\t<RemovalDirList>\n";
   $result = "_A_";
   foreach $key (sort keys %dirList) {
       if ($dirList{$key} eq "Remove" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<Dir>$key</Dir>\n";
           $result = $key;
       }
   }
  print "\t\t</RemovalDirList>\n";
  print "\t\t<ReleaseDirList>\n";
   $result = "_A_";
   foreach $key (sort keys %dirList) {
       if ($dirList{$key} eq "Release" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<Dir>$key</Dir>\n";
           $result = $key;
       }
   }
  print "\t\t</ReleaseDirList>\n";
  print "\t\t<UnReleaseDirList>\n";
   $result = "_A_";
   foreach $key (sort keys %dirList) {
       if ($dirList{$key} eq "UnRelease" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<Dir>$key</Dir>\n";
           $result = $key;
       }
   }
  print "\t\t</UnReleaseDirList>\n";
 print "\t</DirList>\n";

 print "\t<FileList>\n";
  print "\t\t<ReleaseFileList>\n";
   $result = "_A_";
   foreach $key (sort keys %fileList) {
       if ($fileList{$key} eq "Release" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<File>$key</File>\n";
           $result = $key;
       }
   }
  print "\t\t</ReleaseFileList>\n";
  print "\t\t<UnReleaseFileList>\n";
   $result = "_A_";
   foreach $key (sort keys %fileList) {
       if ($fileList{$key} eq "UnRelease" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<File>$key</File>\n";
           $result = $key;
       }
   }
  print "\t\t</UnReleaseFileList>\n";
 print "\t</FileList>\n";

 print "\t<FrameworkRelease>\n";
  print "\t\t<SourceList>\n";
   $result = "_A_";
   foreach $key (sort keys %frameworkList) {
       if ($frameworkList{$key} eq "Source" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<Source>$key</Source>\n";
           $result = $key;
       }
   }
  print "\t\t</SourceList>\n";
  print "\t\t<BINList>\n";
   $result = "_A_";
   foreach $key (sort keys %frameworkList) {
       if ($frameworkList{$key} eq "Binary" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<Binary>$key</Binary>\n";
           $result = $key;
       }
   }
  print "\t\t</BINList>\n";
  print "\t\t<PartialSourceList>\n";
   foreach $key (sort keys %frameworkList) {
       if (($frameworkList{$key} ne "Source") && ($frameworkList{$key} ne "Binary")) {
           my @data = split(':', $frameworkList{$key});
           my $base = (split("#", $key))[0];
           my $module = (split("#", $key))[1];
           print "\t\t\t<PartialSource module=\"${module}\" base=\"${base}\">\n";
            $result = "_A_";
            foreach $binary (@data) {
                if ("$binary/" !~ m#^$result/#) {
                    print "\t\t\t\t<Binary>$binary</Binary>\n";
                    $result = $binary;
                }
            }
           print "\t\t\t</PartialSource>\n";
       }
   }
  print "\t\t</PartialSourceList>\n";
 print "\t</FrameworkRelease>\n";

 print "\t<AndroidRelease>\n";
  print "\t\t<SourceList>\n";
   $result = "_A_";
   foreach $key (sort keys %androidList) {
       if ($androidList{$key} eq "Source" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<Source>$key</Source>\n";
           $result = $key;
       }
   }
  print "\t\t</SourceList>\n";
  print "\t\t<BINList>\n";
   $result = "_A_";
   foreach $key (sort keys %androidList) {
       if ($androidList{$key} eq "Binary" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<Binary>$key</Binary>\n";
           $result = $key;
       }
   }
  print "\t\t</BINList>\n";
 print "\t</AndroidRelease>\n";

 print "\t<APPRelease>\n";
  print "\t\t<SourceList>\n";
   $result = "_A_";
   foreach $key (sort keys %appList) {
       if ($appList{$key} eq "Source" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<Source>$key</Source>\n";
           $result = $key;
       }
   }
  print "\t\t</SourceList>\n";
  print "\t\t<BINList>\n";
   $result = "_A_";
   foreach $key (sort keys %appList) {
       if ($appList{$key} eq "Binary" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<Binary>$key</Binary>\n";
           $result = $key;
       }
   }
  print "\t\t</BINList>\n";
 print "\t</APPRelease>\n";

 print "\t<KernelRelease>\n";
  print "\t\t<SourceList>\n";
   $result = "_A_";
   foreach $key (sort keys %kernelList) {
       if ($kernelList{$key} eq "Source" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<Source>$key</Source>\n";
           $result = $key;
       }
   }
  print "\t\t</SourceList>\n";
  print "\t\t<BINList>\n";
   $result = "_A_";
   foreach $key (sort keys %kernelList) {
       if ($kernelList{$key} eq "Binary" && "$key/" !~ m#^$result/#) {
           print "\t\t\t<Binary>$key</Binary>\n";
           $result = $key;
       }
   }
  print "\t\t</BINList>\n";
 print "\t</KernelRelease>\n";

print "</ReleasePackage>\n";
exit 0;

sub usage{
  warn << "__END_OF_USAGE";
Usage: perl policygen.pl [options]

Options:
  -rel      : release policy file.
  -p        : project to release.
  -c        : customer to release.

Example:
  perl policygen.pl -rel rel_customer_basic,rel_customer_platform_mt6752,rel_customer_modem -p k2v1 -c mediatek

__END_OF_USAGE

  exit 1;

}


