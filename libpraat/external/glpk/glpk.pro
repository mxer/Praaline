! include( ../../common.pri ) {
    error( Could not find the common.pri file! )
}

CONFIG += staticlib
TARGET = glpk

SOURCES = amd_1.c amd_2.c amd_aat.c amd_control.c amd_defaults.c amd_info.c \
   amd_order.c amd_post_tree.c amd_postorder.c amd_preprocess.c amd_valid.c \
   colamd.c \
   glpapi01.c glpapi02.c glpapi03.c glpapi04.c glpapi05.c glpapi06.c glpapi07.c \
   glpapi08.c glpapi09.c glpapi10.c glpapi11.c glpapi12.c glpapi13.c glpapi14.c \
   glpapi15.c glpapi16.c glpapi17.c glpapi18.c glpapi19.c \
   glpavl.c glpbfd.c glpbfx.c glpcpx.c glpdmp.c glpdmx.c \
   glpenv01.c glpenv02.c glpenv03.c glpenv04.c \
   glpenv05.c glpenv06.c glpenv07.c glpenv08.c \
   glpfhv.c glpgmp.c glphbm.c glpini01.c glpini02.c \
   glpios01.c glpios02.c glpios03.c glpios04.c glpios05.c glpios06.c \
   glpios07.c glpios08.c glpios09.c glpios10.c glpios11.c glpios12.c \
   glpipm.c glplib01.c glplib02.c glplib03.c glplpf.c \
   glplpx01.c glplpx02.c glplpx03.c \
   glpluf.c glplux.c glpmat.c \
   glpmpl01.c glpmpl02.c glpmpl03.c glpmpl04.c glpmpl05.c glpmpl06.c glpmps.c \
   glpnet01.c glpnet02.c glpnet03.c glpnet04.c glpnet05.c \
   glpnet06.c glpnet07.c glpnet08.c glpnet09.c \
   glpnpp01.c glpnpp02.c glpnpp03.c glpnpp04.c glpnpp05.c \
   glpqmd.c glprgr.c glprng01.c glprng02.c \
   glpscf.c glpscl.c glpsdf.c glpspm.c glpspx01.c glpspx02.c \
   glpsql.c glpssx01.c glpssx02.c glptsp.c

HEADERS += \
    amd.h \
    amd_internal.h \
    colamd.h \
    glpapi.h \
    glpavl.h \
    glpbfd.h \
    glpbfx.h \
    glpdmp.h \
    glpenv.h \
    glpfhv.h \
    glpgmp.h \
    glphbm.h \
    glpios.h \
    glpipm.h \
    glpk.h \
    glplib.h \
    glplpf.h \
    glpluf.h \
    glplux.h \
    glpmat.h \
    glpmpl.h \
    glpnet.h \
    glpnpp.h \
    glpqmd.h \
    glprgr.h \
    glprng.h \
    glpscf.h \
    glpspm.h \
    glpspx.h \
    glpsql.h \
    glpssx.h \
    glpstd.h \
    glptsp.h
