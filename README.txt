== gr-edacs ==

Author: Clayton Caron

Description:
    The purpose of this project is to provide an easy to use block for tracking 
    EDACS trunked radio communications. It can be used with software defined 
    radio's such as Nuand's bladeRF or the RTL-SDR in conjuction with the 
    OsmoSDR block.

Dependencies:
    GNU Radio >= v3.7
    BOOST
    CPPUNIT
    SWIG
    gr-dsd (github.com/argilo/gr-dsd)

Installation:
    cd gr-edacs
    mkdir build
    cd build
    cmake ../
    make
    sudo make install
    sudo ldconfig
