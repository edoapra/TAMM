
include(TargetMacros)
# add_mpi_unit_test(Test_CD_SVD 2 "${CMAKE_SOURCE_DIR}/../inputs/h2o.nwx")
# add_mpi_unit_test(Test_CD_SVD_GA 2 "${CMAKE_SOURCE_DIR}/../inputs/h2o.nwx")
# add_mpi_unit_test(Test_CCSD_CD 2 "${CMAKE_SOURCE_DIR}/../inputs/h2o.nwx")
add_mpi_unit_test(Test_CCSD_CD_RM 2 "${CMAKE_SOURCE_DIR}/../inputs/h2o.nwx")
# add_mpi_unit_test(Test_CCSD_CD_SVD 2 "${CMAKE_SOURCE_DIR}/../inputs/h2o.nwx")
add_mpi_unit_test(Test_CCSD_Spin 2 "${CMAKE_SOURCE_DIR}/../inputs/h2o.nwx")
add_mpi_unit_test(Test_CCSD_CD_RM_NK 2 "${CMAKE_SOURCE_DIR}/../inputs/h2o.nwx")

if(NWX_CUDA)
    set(CCSD_T_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/ccsd/ccsd_t)
    set(CCSD_T_CUDA_SRCS 
        ${CCSD_T_SRCDIR}/memory.cu  
        ${CCSD_T_SRCDIR}/header.hpp
        ${CCSD_T_SRCDIR}/hybrid.cpp
        ${CCSD_T_SRCDIR}/util_getppn.cpp
        ${CCSD_T_SRCDIR}/ccsd_t_gpu.hpp
        ${CCSD_T_SRCDIR}/ccsd_t_singles_gpu.hpp
        ${CCSD_T_SRCDIR}/ccsd_t_doubles_gpu.hpp
        )
    set(CCSD_T_NWC_SRCS ${CCSD_T_CUDA_SRCS} ${CCSD_T_SRCDIR}/sd_t_total.cu)
    set(CCSD_T_TGEN_SRCS ${CCSD_T_CUDA_SRCS} 
            ${CCSD_T_SRCDIR}/sd_t_total_tgen.cu)
    set(CCSD_T_TGEN_FUSED_SRCS ${CCSD_T_CUDA_SRCS} 
            ${CCSD_T_SRCDIR}/sd_t_total.cu
            ${CCSD_T_SRCDIR}/direct_sd1.cu
            ${CCSD_T_SRCDIR}/direct_sd2.cu)            
    add_mpi_cuda_unit_test(Test_CCSD_T "${CCSD_T_NWC_SRCS}" 2 "${CMAKE_SOURCE_DIR}/../inputs/h2o.nwx")
    add_mpi_cuda_unit_test(Test_CCSD_T_tgen "${CCSD_T_TGEN_SRCS}" 2 "${CMAKE_SOURCE_DIR}/../inputs/h2o.nwx")
    add_mpi_cuda_unit_test(Test_CCSD_T_tgen_fused "${CCSD_T_TGEN_FUSED_SRCS}" 2 "${CMAKE_SOURCE_DIR}/../inputs/h2o.nwx")
endif()
