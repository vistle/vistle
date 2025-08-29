#!/usr/bin/env python
from __future__ import print_function
import contextlib
import os
import platform
import re
import subprocess
import sys

this_dir = os.path.dirname(os.path.abspath(__file__))

results_summary_filename = os.path.join(this_dir, 'examples-buildandtestsummary.txt')
build_detail_filename = os.path.join(this_dir, 'examples-builddetail.txt')
execution_detail_filename = os.path.join(this_dir, 'examples-executiondetail.txt')
mpi_execution_detail_filename = os.path.join(this_dir, 'examples-mpi-executiondetail.txt')

for filename in [results_summary_filename, build_detail_filename, execution_detail_filename, mpi_execution_detail_filename]:
    if os.path.isfile(filename):
        os.remove(filename)

data_filename_pattern = re.compile('.s?z?plt$')
for root, dirs, files in os.walk('.'):
    for f in files:
        if data_filename_pattern.search(f):
            os.remove(os.path.join(root, f))

#
# On the mac it is sometimes the case that the wrapper for the compiler
# is available but no guts underneath.  First check if the wrapper is there
# and then see if it can do anything.
#
def compiler_is_available(compiler):
    is_available = True
    if sys.version_info < (3,):
        from distutils import spawn
        executable = spawn.find_executable(compiler)
    else:
        from shutil import which
        executable = which(compiler)
    if not executable:
        is_available = False
    else:
        if platform.system() == "Windows":
            arg = '/help'
        else:
            arg = '--version'
        try:
            with open(os.devnull, 'r+') as devnull: # Python 3 gives us subprocess.DEVNULL
                subprocess.check_call([executable, arg], stdin=devnull, stdout=devnull, stderr=devnull)
        except:
            print('Info: {} ({} {}) does not execute correctly. Related examples will not be built.'.format(compiler, executable, arg))
            is_available = False
    return is_available

if platform.system() == "Windows":
    fortran_available = compiler_is_available('ifort')
    cpp_available = compiler_is_available('cl')
    mpifortran_available = fortran_available
    mpicpp_available = cpp_available
    mpiexec = r'C:\Program Files\Microsoft MPI\bin\mpiexec.exe'
else:
    fortran_available = compiler_is_available('gfortran')
    cpp_available = compiler_is_available('g++')
    mpifortran_available = compiler_is_available('mpif90')
    mpicpp_available = compiler_is_available('mpic++')
    mpiexec = 'mpiexec'
mpiexec_available = compiler_is_available(mpiexec)

disable_flags = ''
if not fortran_available:
    disable_flags = 'DISABLE_FORTRAN=YES'

if not mpifortran_available:
    disable_flags += ' DISABLE_MPIFORTRAN=YES'

if not mpicpp_available:
    disable_flags += ' DISABLE_MPICPP=YES'

#
# Note: cpp_available is special.  If that's not available then just skip doing all tests...
#

if len(sys.argv) > 1:
    # Specified subdirectories
    dirs = sys.argv[1:]
else:
    # All immediate subdirectories
    dirs = []
    for root, dirnames, filenames in os.walk(this_dir):
        dirs.extend(dirnames)
        break
dirs.sort()

tecplot_home_dir = os.path.dirname(os.path.dirname(os.path.dirname(this_dir)))
if platform.system() == 'Linux':
    os.environ['LD_LIBRARY_PATH'] = os.path.join(tecplot_home_dir, 'bin')
elif platform.system() == 'Windows':
    os.environ['PATH'] = os.path.join(tecplot_home_dir, 'bin') + ';' + os.environ['PATH']
else:
    os.environ['DYLD_LIBRARY_PATH'] = os.path.join(tecplot_home_dir, 'Frameworks')

# Disable Infiniband for MPI apps to avoid warning messages--
# instruct OpenMPI's Modular Component Architecture's Byte Transfer Layer
# to use all available transports except InfiniBand:
os.environ['OMPI_MCA_btl'] = '^openib'

def print_running(execution_detail_file, results_summary_file, exename, executable, isMPI, arg=None):
    if os.path.isfile(executable) and os.access(executable, os.X_OK) and ((not isMPI) or mpiexec_available):
        if arg:
            command = '{} {}'.format(exename, arg)
        else:
            command = exename
        message = '  Running:        {:<58}'.format(command)
        execution_detail_file.write('{}\n'.format(message))
        results_summary_file.write('{}:'.format(message))
    else:
        message = '  Not run:        {:<58}\n'.format(exename)
        execution_detail_file.write(message)
        results_summary_file.write(message)
    execution_detail_file.flush()
    results_summary_file.flush()

def print_result_to_summary_file(results_summary_file, return_code):
    if return_code == 0:
        results_summary_file.write('Passed\n')
    else:
        results_summary_file.write('Error\n')

def runit(exename, isMPI, execution_detail_file, results_summary_file):
    env=os.environ.copy()
    if platform.system() == "Windows":
        env['PATH'] = r'..\..\..\..\bin'
        if isMPI:
            executable = os.path.join('x64', 'ReleaseMPI', '{}mpi.exe'.format(exename))
        else:
            executable = os.path.join('x64', 'Release', '{}.exe'.format(exename))
    else:
        env['DYLD_LIBRARY_PATH'] = '../../../../Frameworks'
        env['LD_LIBRARY_PATH'] = '../../../../bin'
        if isMPI:
            executable = os.path.join('.', exename + 'mpi')
        else:
            executable = os.path.join('.', exename)
    print_running(execution_detail_file, results_summary_file, exename, executable, isMPI)
    if os.path.isfile(executable) and os.access(executable, os.X_OK):
        if isMPI:
            returncode = subprocess.call([mpiexec, '-n', '3', executable], stdout=execution_detail_file, stderr=execution_detail_file, env=env)
        else:
            returncode = subprocess.call(executable, stdout=execution_detail_file, stderr=execution_detail_file, env=env)
        print_result_to_summary_file(results_summary_file, returncode)

def is_tecplot_home_dir_valid():
    if os.path.isfile(os.path.join(tecplot_home_dir, 'include', 'TECIO.h')) or \
        os.path.isfile(os.path.join(os.path.dirname(this_dir), 'teciosrc', 'TECIO.h')):
        return True
    return False

@contextlib.contextmanager
def pushd(new_dir):
    previous_dir = os.getcwd()
    os.chdir(new_dir)
    try:
        yield
    finally:
        os.chdir(previous_dir)


with open(results_summary_filename, 'w') as results_summary_file, \
    open(execution_detail_filename, 'w') as execution_detail_file, \
    open(mpi_execution_detail_filename, 'w') as mpi_execution_detail_file:
    if not cpp_available:
        message = 'No compilers are available. Skipping tecio example testing.'
        print(message) 
        results_summary_file.write('{}\n'.format(message))
    elif is_tecplot_home_dir_valid():
        if platform.system() == 'Windows':
            command_template = 'msbuild {sln_file} /target:Build /property:Configuration={configuration} >>"' + build_detail_filename + '" 2>&1'
        else:
            command_template = 'make {mpiprefix_flag} {disable_flags} >>"' + build_detail_filename + '" 2>&1'
        for dir in dirs:
            with pushd(dir):
                #
                # First build and test mpi if available.   Do this separate from building non-mpi
                # since the output files are all named the same regardless of mpi or non-mpi and
                # then rename the output files with a -mpi suffix to keep them separate from the
                # subsequent non-mpi runs.
                #
                solution_pattern = re.compile('.sln$')
                fortran_solution_pattern = re.compile('f9?0?.sln$')
                if (mpifortran_available or mpicpp_available) and (('partitioned' in dir) or ('ijkmany' in dir)):
                    result = 0
                    if platform.system() == 'Windows':
                        for f in os.listdir('.'):
                            if os.path.isfile(f) and solution_pattern.search(f) and (mpifortran_available or not fortran_solution_pattern.search(f)):
                                command = command_template.format(sln_file=f, configuration='ReleaseMPI')
                                r = subprocess.call(command, shell=True)
                                if r != 0:
                                    result = r
                    else:
                        command = command_template.format(mpiprefix_flag='mpiprefix=mpi', disable_flags=disable_flags)
                        result = subprocess.call(command, shell=True)
                    results_summary_file.write('Building (mpi)  {:<60}:'.format(dir))
                    if result == 0:
                        results_summary_file.write('Passed\n')
                        runit('{}'.format(dir), True, mpi_execution_detail_file, results_summary_file)
                        runit('{}f90'.format(dir), True, mpi_execution_detail_file, results_summary_file)
                        plt_pattern = re.compile('.s?z?plt$')
                        for file in os.listdir('.'):
                            if plt_pattern.search(file):
                                bname, extension = os.path.splitext(file)
                                os.rename(file, '{}mpi{}'.format(bname, extension))
                    else:
                        results_summary_file.write('Failed\n')
  
                result = 0
                if platform.system() == 'Windows':
                    for f in os.listdir('.'):
                        if os.path.isfile(f) and solution_pattern.search(f) and (fortran_available or not fortran_solution_pattern.search(f)):
                            command = command_template.format(sln_file=f, configuration='Release')
                            r = subprocess.call(command, shell=True)
                            if r != 0:
                                result = r
                else:
                    command = command_template.format(mpiprefix_flag='', disable_flags=disable_flags)
                    result = subprocess.call(command, shell=True)
                results_summary_file.write('Building        {:<60}:'.format(dir))
                if result == 0:
                    results_summary_file.write('Passed\n')
                    if dir != 'rewriteszl':
                        runit(dir, False, execution_detail_file, results_summary_file)
                        runit('{}f'.format(dir), False, execution_detail_file, results_summary_file)
                        runit('{}f90'.format(dir), False, execution_detail_file, results_summary_file)
                else:
                    results_summary_file.write('Failed\n')

        #
        # Run rewriteszl last (if any szplt files were produced)
        # Don't rewrite solution files.
        #
        szplt_files = []
        szplt_pattern = re.compile('.szplt$')
        for root, dirs, files in os.walk('.'):
            for f in files:
                if szplt_pattern.search(f) and not 'solution' in f:
                    szplt_files.append(os.path.normpath(os.path.join(root, f)))
        szplt_files.sort()
        if len(szplt_files) > 0:
            if platform.system() == 'Windows':
                rewriteszl_exe = os.path.join('rewriteszl', 'x64', 'Release', 'rewriteszl.exe')
            else:
                rewriteszl_exe = os.path.join('rewriteszl', 'rewriteszl')
  
            if os.path.isfile(rewriteszl_exe) and os.access(rewriteszl_exe, os.X_OK):
                error_code = 0
                for file in szplt_files:
                    print_running(execution_detail_file, results_summary_file, 'rewriteszl', rewriteszl_exe, False, file)
                    name_body = os.path.splitext(os.path.basename(file))[0]
                    out_file = os.path.join('rewriteszl', '{}-rewritten.szplt'.format(name_body))
                    error_code = subprocess.call([rewriteszl_exe, file, out_file], stdout=execution_detail_file, stderr=execution_detail_file)
                    print_result_to_summary_file(results_summary_file, error_code)
    
        print('results summary is in:      {}'.format(results_summary_filename))
        print('building summary is in:     {}'.format(build_detail_filename))
        print('execution detail is in:     {}'.format(execution_detail_filename))
        print('MPI execution detail is in: {}'.format(mpi_execution_detail_filename))
    else:
        print('{} is not a complete/valid tecplot home directory'.format(tecplot_home_dir))

