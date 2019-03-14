#! /usr/bin/env python
__author__ = 'mtk'
import sys
import os

def main(argv):
    try:
        opts, args = getopt.getopt(argv, "hc:k:p:b:l:m:H:", ["help", "prjconfig=", "kconfig=", "project=", "preloader=", "lk=", "md32=", "header="])
    except getopt.GetoptError:
        Usage()
        sys.exit(2)

    prjConfig, kConfig, project, plConfig, lkConfig, md32Config, header = \
        parse_opt(opts)
    if not prjConfig or not project:
        print >> sys.stderr, "[Error] prjConfig and project are both required"
        Usage()
    if not kConfig and not plConfig and not lkConfig and not md32Config and \
        not header:
        print >> sys.stderr, "[Error] Missing check targets"
        Usage()

    check_path(prjConfig)
    prj_option = get_projectConfiguration(prjConfig)
    reCode = 0

    if kConfig:
        check_path(kConfig)
        k_option = get_kconfig(kConfig)
        reCode += run_gen_defconfig(prj_option, k_option)
    if plConfig:
        check_path(plConfig)
        pl_option = get_pl_config(plConfig)
        reCode += run_check_pl_config(prj_option, pl_option)
    if lkConfig:
        check_path(lkConfig)
        lk_option = get_lk_config(lkConfig)
        reCode += run_check_lk_config(prj_option, lk_option)
    if md32Config:
        check_path(md32Config)
        md32_option = get_md32_config(md32Config)
        reCode += run_check_md32_config(prj_option, md32_option)
    if header:
        for p in header.split(','):
            p = os.path.expanduser(p)
            check_path(p)
            header_option = get_header_config(p)
            reCode += run_check_header_config(prj_option, header_option, p)

    sys.exit(reCode)


def check_path(path):
    if not os.path.exists(path):
        print >> sys.stderr, "[Error] Can not find out the file %s" % path
        sys.exit(12)

def parse_opt(opts):
    prjconfig = ""
    kconfig = ""
    project = ""
    plconfig = ""
    lkconfig = ""
    md32config = ""
    header = ""
    for opt, arg in opts:
        if opt in ("-c", "--prjconfig"):
            prjconfig = arg
        if opt in ("-k", "--kconfig"):
            kconfig = arg
        if opt in ("-p", "--project"):
            project = arg
        if opt in ("-b", "--preloader"):
            plconfig = arg
        if opt in ("-l", "--lk"):
            lkconfig = arg
        if opt in ("-m", "--md32"):
            md32config = arg
        if opt in ("-H", "--header"):
            header = arg
        if opt in ("-h", "--help"):
            Usage()
    return prjconfig, kconfig, project, plconfig, lkconfig, md32config, header


def get_projectConfiguration(prjConfig):
    """query the current platform"""
    pattern = [re.compile("^([^=\s]+)\s*=\s*(.+)$"),
               re.compile("^([^=\s]+)\s*=$")]
    config = {}
    ff = open(prjConfig, "r")
    for line in ff.readlines():
        result = (filter(lambda x: x, [x.search(line) for x in pattern]) or [None])[0]
        if not result: continue
        name, value = None, None
        if len(result.groups()) == 0: continue
        name = result.group(1)
        try:
            value = result.group(2)
        except IndexError:
            value = ""
        config[name] = value.strip()
        #for debug
        #print >> sys.stdout, "config name:%s config value: %s \n" % (name, value)
    return config


def run_gen_defconfig(prjOption, kOption):
    """Generate defconfig options base on ProjectConfig.mk"""

    genKconfig = []
    return_code = 0
    for i in prjOption:
        #print >>sys.stdout,'config_NAME:%s config_VALUE:%s' % (i,prjOption[i])
        if prjOption[i] == 'yes':
            #print 'CONFIG_' + i + '=y'
            #genKconfig.append('CONFIG_' + i + '=y')
            if 'CONFIG_'+i in kOption:
                if kOption['CONFIG_'+i] != 'y':
                    print >> sys.stdout, "Kconfig Setting: %s" % kOption['CONFIG_'+i]
                    print >> sys.stdout, "ProjectConfig Setting: %s" % prjOption[i]
                    print >> sys.stdout, "*** Boolean ERROR ***: CONFIG_%s not sync with %s in ProjectConfig.mk " % (i,i)
                    return_code += 1
        elif prjOption[i] == 'no':
            #print >>sys.stdout, '# CONFIG_' + i + ' is not set'
            #genKconfig.append('# CONFIG_' + i + ' is not set')
            if 'CONFIG_'+i in kOption:
                if kOption['CONFIG_'+i] != 'is not set':
                    print >> sys.stdout, "Kconfig Setting: %s" % kOption['CONFIG_'+i]
                    print >> sys.stdout, "ProjectConfig Setting: %s" % prjOption[i]
                    print >> sys.stdout, "*** Boolean ERROR ***: CONFIG_%s not sync with %s in ProjectConfig.mk" % (i,i)
                    return_code += 1
        elif prjOption[i] =='':
            if 'CONFIG_'+i in kOption:
                if kOption['CONFIG_'+i] != 'is not set' and  kOption['CONFIG_'+i] != '\"\"':
                    print >> sys.stdout, "Kconfig Setting: %s" % kOption['CONFIG_'+i]
                    print >> sys.stdout, "ProjectConfig Setting: %s" % prjOption[i]
                    print >> sys.stdout, "*** Boolean ERROR ***: CONFIG_%s not sync with %s in ProjectConfig.mk" % (i,i)
                    return_code += 1

        elif prjOption[i].isdigit():
            #print >> sys.stdout, 'CONFIG_' + i + '=%s' % prjOption[i]
            #genKconfig.append('CONFIG_' + i + '=%s' % prjOption[i])
            if 'CONFIG_'+i in kOption:
                if kOption['CONFIG_'+i] != prjOption[i] and kOption['CONFIG_'+i] != '\"'+prjOption[i]+'\"':
                    print >> sys.stdout, "Kconfig Setting: %s" % kOption['CONFIG_'+i]
                    print >> sys.stdout, "ProjectConfig Setting: %s" % prjOption[i]
                    print >> sys.stdout, "*** Int ERROR ***: CONFIG_%s not sync with %s in ProjectConfig.mk" % (i,i)
                    return_code += 1
        elif len(prjOption[i]) > 0:
            pattern = re.compile('^0x\w*')
            match = pattern.match(prjOption[i])
            if match:
                #print >> sys.stdout, 'CONFIG_' + i + '=%s' % prjOption[i]
                #genKconfig.append('CONFIG_' + i + '=%s' % prjOption[i])
                if 'CONFIG_'+i in kOption:
                    if for_hex_parsing(kOption['CONFIG_'+i]) != for_hex_parsing(prjOption[i]) and kOption['CONFIG_'+i] != '\"'+prjOption[i]+'\"':
                        print >> sys.stdout, "Kconfig Setting: %s" % kOption['CONFIG_'+i]
                        print >> sys.stdout, "ProjectConfig Setting: %s" % prjOption[i]
                        print >> sys.stdout, "*** Hex ERROR ***: CONFIG_%s not sync with %s in ProjectConfig.mk" % (i,i)
                        return_code += 1
            else:
                #print >> sys.stdout, 'CONFIG_'+i+'=\"%s\"' % prjOption[i]
                #genKconfig.append('CONFIG_'+i+'\"=%s\"' % prjOption[i])
                if 'CONFIG_'+i in kOption:
                    if kOption['CONFIG_'+i].lower() != '\"'+prjOption[i].lower()+'\"' and kOption['CONFIG_'+i] !='y':
                        print >> sys.stdout, "Kconfig Setting: %s" % kOption['CONFIG_'+i]
                        print >> sys.stdout, "ProjectConfig Setting: %s" % prjOption[i]
                        print >> sys.stdout, "*** String ERROR ***: CONFIG_%s not sync with %s in ProjectConfig.mk" % (i,i)
                        return_code += 1
    return return_code


def run_check_pl_config(prjOption, plOption):
    return_code = 0
    for i in prjOption:
        if i in plOption:
            if plOption[i] != prjOption[i]:
                print >> sys.stdout, "Preloader config Setting: %s" % plOption[i]
                print >> sys.stdout, "ProjectConfig Setting: %s" % prjOption[i]
                print >> sys.stdout, "*** String ERROR ***: %s not sync with %s in ProjectConfig.mk " % (i,i)
                return_code += 1
    return return_code


def run_check_lk_config(prjOption, lkOption):
    return_code = 0
    for i in prjOption:
        if i in lkOption:
            if lkOption[i] != prjOption[i] and lkOption[i] != '\"'+prjOption[i]+'\"':
                print >> sys.stdout, "LK config Setting: %s" % lkOption[i]
                print >> sys.stdout, "ProjectConfig Setting: %s" % prjOption[i]
                print >> sys.stdout, "*** String ERROR ***: %s not sync with %s in ProjectConfig.mk " % (i,i)
                return_code += 1
    return return_code


def run_check_md32_config(prjOption, md32Option):
    return_code = 0
    for i in prjOption:
        if i in md32Option:
            if md32Option[i] != prjOption[i]:
                print >> sys.stdout, "MD32 config Setting: %s" % md32Option[i]
                print >> sys.stdout, "ProjectConfig Setting: %s" % prjOption[i]
                print >> sys.stdout, "*** String ERROR ***: %s not sync with %s in ProjectConfig.mk " % (i,i)
                return_code += 1
    return return_code

def run_check_header_config(prjOption, hdrOption, headerFile):
    return_code = 0
    for i in prjOption:
        if i in hdrOption:
            if hdrOption[i][0] != prjOption[i]:
                print >> sys.stdout, "Header config Setting: %s" % \
                                     hdrOption[i][0]
                print >> sys.stdout, "ProjectConfig Setting: %s" % prjOption[i]
                print >> sys.stdout, "Header file/line no  : %s:%d" % \
                                     (headerFile, hdrOption[i][1])
                print >> sys.stdout, "*** String ERROR ***: %s not sync with" \
                                     " %s in ProjectConfig.mk" % (i, i)
                return_code += 1
    return return_code


def for_hex_parsing(hex):
    pattern = re.compile("0x[0]*([A-Za-z1-9]+0*)")
    match= pattern.match(hex)
    if match:
        return match.group(1)


def get_kconfig(kconfig):
    """read all the kernel config for furture comparasion
    direct use the comparation result as error message"""

    kconfig_option={}
    pattern = [re.compile("^([^=\s]+)\s*=\s*(.+)$"),
               re.compile("^#\s(\w+) is not set")]
    ff = open(kconfig,'r')
    for line in ff.readlines():
        result = (filter(lambda x: x, [x.search(line) for x in pattern]) or [None])[0]
        if not result: continue
        name, value = None, None
        if len(result.groups()) == 0: continue
        name = result.group(1)
        try:
            value = result.group(2)
        except IndexError:
             value = " is not set"
        kconfig_option[name] = value.strip()
        #for debug
        #print >> sys.stdout, "config name:%s config value: %s \n" % (name, value)
    return kconfig_option


def get_pl_config(plConfig):
    pattern = [re.compile("^([^\#\:=\s]+)\s*\:?=\s*(.*)$")]
    config = {}
    ff = open(plConfig, "r")
    for line in ff.readlines():
        result = (filter(lambda x: x, [x.search(line) for x in pattern]) or [None])[0]
        if not result: continue
        name, value = None, None
        if len(result.groups()) == 0: continue
        name = result.group(1)
        try:
            value = result.group(2)
        except IndexError:
            value = ""
        config[name] = value.strip()
    return config


def get_lk_config(lkConfig):
    return get_pl_config(lkConfig)


def get_md32_config(md32Config):
    return get_pl_config(md32Config)

def get_header_config(header):
    """
Parse the C/C++ header file that contains feature options.

Each feature option is defined in the format:
#define <FO_NAME> [<FO_VALUE>]

Entry value mapping table:
    +==============================+========================+
    | Header file entry            | ProjectConfig.mk entry |
    +==============================+========================+
    | #define (CFG_)?FOO           | FOO = yes              |
    +------------------------------+------------------------+
    | /* (CFG_)?FOO is not set */  | FOO = no               |
    +------------------------------+------------------------+
    | #define (CFG_)?FOO VAL       | FOO = VAL              |
    +------------------------------+------------------------+
    """

    pattern = [re.compile("^#define\s+(?:CFG_)?(\S+)(?:\s+(\S+))?$")]
    negation_pattern = re.compile("^/\*\s*(?:CFG_)?(\S+) is not set\s*\*/$")
    config = {}

    # Usually the first "#ifndef xxx" contains the macro that wraps the whole
    # header file to avoid multiple inclusion. The problem is that the next
    # line, "#define xxx", is in the same format as FO definitions.
    # This non-FO wrapper macro needs to be excluded.
    wrapper_macro_pattern = re.compile("^#ifndef\s+(\S+)$")
    wrapper_macro_found = False
    wrapper_macro_name = ''

    with open(header, "r") as f:
        # If we are in multi-line comment section
        ignored = False
        # Put line number of each FO for the ease of debugging.
        lno = 0

        for line in f.readlines():
            lno = lno + 1
            line = line.strip()

            # If we are in multi-line comment body, see if we can find the end
            # mark here. If so, harvest the following strings.
            if ignored:
                ml_comment_end = line.find("*/")
                if (ml_comment_end >= 0):
                    # 3: length of "*/" + 1 char afterwards
                    line = line[ml_comment_end + 3:]
                    line = line.strip()
                    ignored = False

            if ignored: continue

            # Remove anything after double-dash comment (//)
            dd_comment = line.find("//")
            if dd_comment >= 0:
                line = line[:dd_comment]
                line = line.strip()
                # print "== Line with // comment: \"%s\"" % line

            # Multi-line comment treatment
            ml_comment_start = line.find("/*")
            if (ml_comment_start >= 0):
                # Look for negation entry
                result = negation_pattern.search(line)
                if result:
                    config[result.group(1)] = ("no", lno)
                    continue

                ml_comment_end = line.find("*/")
                if ml_comment_end >= 0:
                    line = line[:ml_comment_start] + line[ml_comment_end + 3:]
                    # print "== Line with inline comments: \"%s\"" % line
                else:
                    line = line[:ml_comment_start]
                    ignored = True

            line = line.strip()
            if len(line) == 0: continue

            # Wrapper macro detection
            if not wrapper_macro_found and line.startswith("#ifndef"):
                result = wrapper_macro_pattern.search(line)
                if result:
                    wrapper_macro_found = True
                    # print "Wrapper macro found: %s" % result.group(1)
                    wrapper_macro_name = result.group(1)
                else:
                    print >> sys.stderr, "[Error] Illegal #ifndef syntax " \
                             "found in %s" % header
                    sys.exit(1)

            result = (filter(None, [x.search(line) for x in pattern])
                      or [None])[0]
            if not result: continue

            name, value = None, None
            if len(result.groups()) == 0: continue
            name = result.group(1)

            if result.group(2):
                value = result.group(2).strip('"')
            else:
                # For the pattern "#ifdef FOO", suppose the value is "yes"
                # in device ProjectConfig.mk
                value = "yes"

            config[name] = (value.strip(), lno)

    if wrapper_macro_name:
        del config[wrapper_macro_name]

    # for i in config.keys():
    #     print "%s -> %s" % (i, config[i][0])

    return config

def Usage():
    print """
Usage:
    -h, --help=:       show the Usage of commandline argments;
    -c, --prjconfig=:  project configuration path;
    -k, --kconfig=:    kconfiguration path;
    -b, --preloader=:  preloader configuration path;
    -l, --lk=:         lk configuration path;
    -m, --md32=:       md32 configuration path;
    -p, --project=:    check project.
    -H, --header=:     check C/C++ header files. Separate multiple paths of
                       header files with comma (,).

Example:
    $ python check_kernel_config.py -c $prjconfig_path -k $kconfig_path -b $pl_path -l $lk_path -p $project -H $header1[,$header2...]
    $ python check_kernel_config.py --prjconfig $prjconfig_path --kconfig $kconfig_path --preloader $pl_path --lk $lk_path --poject $project --header $header1[,$header2...]
    """
    sys.exit(1)


if __name__ == "__main__":
    import os, sys, re, getopt, commands, string
    import xml.dom.minidom as xdomxdom
    main(sys.argv[1:])
