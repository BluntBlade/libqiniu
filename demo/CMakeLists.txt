add_executable (qcdnurl qcdnurl.c)
target_link_libraries (qcdnurl qiniu crypto)

add_executable (qdnurl qdnurl.c)
target_link_libraries (qdnurl qiniu crypto)

add_executable (qacctoken qacctoken.c)
target_link_libraries (qacctoken qiniu crypto)

add_executable (qstat qstat.c)
target_link_libraries (qstat qiniu curl crypto)

add_executable (qbatch_stat qbatch_stat.c)
target_link_libraries (qbatch_stat qiniu curl crypto)

add_executable (qlist qlist.c)
target_link_libraries (qlist qiniu curl crypto)

add_executable (qcopy qcopy.c)
target_link_libraries (qcopy qiniu curl crypto)

add_executable (qmove qmove.c)
target_link_libraries (qmove qiniu curl crypto)

add_executable (qdelete qdelete.c)
target_link_libraries (qdelete qiniu curl crypto)

add_executable (qchgm qchgm.c)
target_link_libraries (qchgm qiniu curl crypto)

add_executable (qfetch qfetch.c)
target_link_libraries (qfetch qiniu curl crypto)

add_executable (qputf qputf.c)
target_link_libraries (qputf qiniu curl crypto)

add_executable (qputfa qputfa.c)
target_link_libraries (qputfa qiniu curl crypto)

add_executable (qputfb qputfb.c)
target_link_libraries (qputfb qiniu curl crypto)

add_executable (qregion qregion.c)
target_link_libraries (qregion qiniu curl crypto)

add_executable (qeputf qeputf.c)
target_link_libraries (qeputf qiniu curl crypto)

add_executable (quphf quphf.c)
target_link_libraries (quphf qiniu curl crypto)
