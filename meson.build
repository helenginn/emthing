project('em_thing', 'cpp', 'c')
qt5 = import('qt5')

qt5_dep = dependency('qt5', modules: ['Core', 'Gui', 'Widgets'], required : true)
dep_gl = dependency('gl', required : true)
png_dep = dependency('libpng')
helen3d = dependency('helen3d')
helencore = dependency('helencore')
dep_vag = dependency('libvag')
dep_vgui = dependency('libvgui')
fftw_dep = dependency('fftw3f')
dep_vgeom = dependency('libvgeom', required: false)
dep_ccp4 = dependency('libccp4')
dep_c4x = dependency('libcluster4x')
dep_ica = dependency('libica')

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

# Run the version and hash generator
hash_script = find_program('get_hash.sh')

gen_src = custom_target('commit-output',
                        input : ['src/commit.h.pre'],
                        output : ['commit.h'],
                        command : [hash_script, '@INPUT@', '@OUTPUT@'],
                        build_by_default: true,
                        build_always_stale: true)

# Graphical user interface
# (including all the Qt and OpenGL stuff)

# Pre-processing by Qt5

moc_files = qt5.preprocess(moc_headers : [
'src/Aligner.h', 
'src/DensityDisplay.h', 
'src/Trace.h', 
'src/View.h', 
'src/VolumeView.h', 
],
		moc_extra_arguments: ['-DMAKES_MY_MOC_HEADER_COMPILE'])

executable('em_thing', gen_src, moc_files,
'src/Aligner.cpp', 
'src/Control.cpp', 
'src/DensityDisplay.cpp', 
'src/DistortMap.cpp', 
'src/MRCin.cpp', 
'src/MRCImage.cpp', 
'src/Particle.cpp', 
'src/ParticleGroup.cpp', 
'src/Rotless.cpp', 
'src/Symmetry.cpp', 
'src/Trace.cpp', 
'src/View.cpp', 
'src/VolumeView.cpp', 
'src/main.cpp', 
cpp_args: ['-std=c++11'], dependencies : [ 
qt5_dep, 
dep_gl, 
png_dep,
fftw_dep,
dep_vgui, dep_vag, dep_vgeom, dep_ccp4,
dep_c4x,
helen3d,
helencore
], install: true)

install_data('res/Info.plist', install_dir: 'Contents')
install_data('res/HelloQt.icns', install_dir: 'Contents/Resources')
install_data('res/qt.conf', install_dir: 'Contents/Resources')

subdir('install-scripts')

