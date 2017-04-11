# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Trunked Radio
# Generated: Sun Apr  2 22:54:01 2017
##################################################

from gnuradio import analog
from gnuradio import blocks
from gnuradio import digital
from gnuradio import filter
from gnuradio import gr
from gnuradio.filter import firdes
from grc_gnuradio import blks2 as grc_blks2
import dsd
import edacs
import math
import threading
import time


class trunked_radio(gr.hier_block2):

    def __init__(self, center_freq=867.4105, find_chan_nums=False, freq_list=[866.0375, 866.2875, 866.5375, 866.7875, 867.0375, 867.2875, 867.5375, 867.7875, 868.0375, 868.2875, 868.5375, 868.7875, 866.0625, 866.3125, 866.5625, 866.8125, 867.0625, 867.3125, 867.5625, 867.8125], noise_threshold=5, sink_rate=48000, source_rate=4000000, talkgroup=1796, tone_threshold=0.1, track_analog=True, track_digital=False, voice_threshold=0.4):
        gr.hier_block2.__init__(
            self, "Trunked Radio",
            gr.io_signature(1, 1, gr.sizeof_gr_complex*1),
            gr.io_signaturev(2, 2, [gr.sizeof_float*1, gr.sizeof_char*1]),
        )

        ##################################################
        # Parameters
        ##################################################
        self.center_freq = center_freq
        self.find_chan_nums = find_chan_nums
        self.freq_list = freq_list
        self.noise_threshold = noise_threshold
        self.sink_rate = sink_rate
        self.source_rate = source_rate
        self.talkgroup = talkgroup
        self.tone_threshold = tone_threshold
        self.track_analog = track_analog
        self.track_digital = track_digital
        self.voice_threshold = voice_threshold

        ##################################################
        # Variables
        ##################################################
        self.baud_rate = baud_rate = 9600
        self.samp_per_sym = samp_per_sym = sink_rate / baud_rate
        self.fp_sel_index = fp_sel_index = 0

        ##################################################
        # Blocks
        ##################################################
        self.edacs_handle_eot_0 = edacs.handle_eot(sink_rate, 4800, tone_threshold, noise_threshold)
        
        def _fp_sel_index_probe():
            while True:
                val = self.edacs_handle_eot_0.get_sel_index()
                try:
                    self.set_fp_sel_index(val)
                except AttributeError:
                    pass
                time.sleep(1.0 / (100))
        _fp_sel_index_thread = threading.Thread(target=_fp_sel_index_probe)
        _fp_sel_index_thread.daemon = True
        _fp_sel_index_thread.start()
            
        self.rational_resampler_xxx_1 = filter.rational_resampler_fff(
                interpolation=sink_rate / 8000,
                decimation=1,
                taps=None,
                fractional_bw=None,
        )
        self.rational_resampler_xxx_0_0 = filter.rational_resampler_ccc(
                interpolation=int(sink_rate / 1000),
                decimation=int(source_rate / 100000),
                taps=None,
                fractional_bw=None,
        )
        self.rational_resampler_xxx_0 = filter.rational_resampler_ccc(
                interpolation=int(sink_rate / 1000),
                decimation=int(source_rate / 100000),
                taps=None,
                fractional_bw=None,
        )
        self.low_pass_filter_0_0 = filter.fir_filter_ccf(100, firdes.low_pass(
        	1, source_rate, .00625e6, .002e6, firdes.WIN_HAMMING, 6.76))
        self.low_pass_filter_0 = filter.fir_filter_ccf(100, firdes.low_pass(
        	1, source_rate, .00625e6, .002e6, firdes.WIN_HAMMING, 6.76))
        self.edacs_proc_msg_0 = edacs.proc_msg(talkgroup, (freq_list), center_freq, find_chan_nums, track_analog, track_digital)
        self.edacs_find_chan_nums_0 = edacs.find_chan_nums((freq_list), center_freq, source_rate, voice_threshold)
        self.dsd_block_ff_0 = dsd.block_ff(dsd.dsd_FRAME_PROVOICE,dsd.dsd_MOD_AUTO_SELECT,3,False,0,True,0)
        self.digital_gfsk_demod_0 = digital.gfsk_demod(
        	samples_per_symbol=samp_per_sym,
        	sensitivity=1.0,
        	gain_mu=0.175,
        	mu=0.5,
        	omega_relative_limit=0.005,
        	freq_error=0.0,
        	verbose=False,
        	log=False,
        )
        self.digital_correlate_access_code_bb_0 = digital.correlate_access_code_bb('010101010101010101010111000100100101010101010101', 0)
        self.blocks_unpack_k_bits_bb_0 = blocks.unpack_k_bits_bb(8)
        self.blocks_pack_k_bits_bb_0 = blocks.pack_k_bits_bb(8)
        self.blocks_not_xx_0 = blocks.not_bb()
        self.blocks_multiply_xx_1 = blocks.multiply_vcc(1)
        self.blocks_multiply_xx_0 = blocks.multiply_vcc(1)
        self.blocks_delay_0 = blocks.delay(gr.sizeof_gr_complex*1, 1024)
        self.blks2_selector_1 = grc_blks2.selector(
        	item_size=gr.sizeof_float*1,
        	num_inputs=2,
        	num_outputs=1,
        	input_index=fp_sel_index,
        	output_index=0,
        )
        self.analog_sig_source_x_1 = analog.sig_source_c(source_rate, analog.GR_COS_WAVE, 0, 1, 0)
        self.analog_sig_source_x_0 = analog.sig_source_c(source_rate, analog.GR_COS_WAVE, 0, 1, 0)
        self.analog_quadrature_demod_cf_0 = analog.quadrature_demod_cf(1)

        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.edacs_find_chan_nums_0, 'status_out'), (self.edacs_proc_msg_0, 'chan_status_in'))    
        self.msg_connect((self.edacs_handle_eot_0, 'status_out'), (self.edacs_proc_msg_0, 'eot_status_in'))    
        self.msg_connect((self.edacs_proc_msg_0, 'ctrl_freq'), (self.analog_sig_source_x_0, 'freq'))    
        self.msg_connect((self.edacs_proc_msg_0, 'voice_freq'), (self.analog_sig_source_x_1, 'freq'))    
        self.msg_connect((self.edacs_proc_msg_0, 'chan_status_out'), (self.edacs_find_chan_nums_0, 'status_in'))    
        self.msg_connect((self.edacs_proc_msg_0, 'eot_status_out'), (self.edacs_handle_eot_0, 'status_in'))    
        self.connect((self.analog_quadrature_demod_cf_0, 0), (self.edacs_handle_eot_0, 0))    
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_multiply_xx_0, 1))    
        self.connect((self.analog_sig_source_x_1, 0), (self.blocks_multiply_xx_1, 1))    
        self.connect((self.blks2_selector_1, 0), (self, 0))    
        self.connect((self.blocks_delay_0, 0), (self.edacs_find_chan_nums_0, 0))    
        self.connect((self.blocks_multiply_xx_0, 0), (self.low_pass_filter_0, 0))    
        self.connect((self.blocks_multiply_xx_1, 0), (self.low_pass_filter_0_0, 0))    
        self.connect((self.blocks_not_xx_0, 0), (self.blocks_unpack_k_bits_bb_0, 0))    
        self.connect((self.blocks_pack_k_bits_bb_0, 0), (self.blocks_not_xx_0, 0))    
        self.connect((self.blocks_unpack_k_bits_bb_0, 0), (self.digital_correlate_access_code_bb_0, 0))    
        self.connect((self.digital_correlate_access_code_bb_0, 0), (self.edacs_proc_msg_0, 0))    
        self.connect((self.digital_gfsk_demod_0, 0), (self.blocks_pack_k_bits_bb_0, 0))    
        self.connect((self.dsd_block_ff_0, 0), (self.rational_resampler_xxx_1, 0))    
        self.connect((self.edacs_handle_eot_0, 0), (self.blks2_selector_1, 0))    
        self.connect((self.edacs_handle_eot_0, 0), (self.dsd_block_ff_0, 0))    
        self.connect((self.edacs_proc_msg_0, 0), (self, 1))    
        self.connect((self.low_pass_filter_0, 0), (self.rational_resampler_xxx_0, 0))    
        self.connect((self.low_pass_filter_0_0, 0), (self.rational_resampler_xxx_0_0, 0))    
        self.connect((self, 0), (self.blocks_delay_0, 0))    
        self.connect((self, 0), (self.blocks_multiply_xx_0, 0))    
        self.connect((self, 0), (self.blocks_multiply_xx_1, 0))    
        self.connect((self.rational_resampler_xxx_0, 0), (self.digital_gfsk_demod_0, 0))    
        self.connect((self.rational_resampler_xxx_0_0, 0), (self.analog_quadrature_demod_cf_0, 0))    
        self.connect((self.rational_resampler_xxx_1, 0), (self.blks2_selector_1, 1))    

    def get_center_freq(self):
        return self.center_freq

    def set_center_freq(self, center_freq):
        self.center_freq = center_freq

    def get_find_chan_nums(self):
        return self.find_chan_nums

    def set_find_chan_nums(self, find_chan_nums):
        self.find_chan_nums = find_chan_nums

    def get_freq_list(self):
        return self.freq_list

    def set_freq_list(self, freq_list):
        self.freq_list = freq_list

    def get_noise_threshold(self):
        return self.noise_threshold

    def set_noise_threshold(self, noise_threshold):
        self.noise_threshold = noise_threshold
        self.edacs_handle_eot_0.set_noise_threshold(self.noise_threshold)

    def get_sink_rate(self):
        return self.sink_rate

    def set_sink_rate(self, sink_rate):
        self.sink_rate = sink_rate
        self.set_samp_per_sym(self.sink_rate / self.baud_rate)

    def get_source_rate(self):
        return self.source_rate

    def set_source_rate(self, source_rate):
        self.source_rate = source_rate
        self.low_pass_filter_0_0.set_taps(firdes.low_pass(1, self.source_rate, .00625e6, .002e6, firdes.WIN_HAMMING, 6.76))
        self.low_pass_filter_0.set_taps(firdes.low_pass(1, self.source_rate, .00625e6, .002e6, firdes.WIN_HAMMING, 6.76))
        self.analog_sig_source_x_1.set_sampling_freq(self.source_rate)
        self.analog_sig_source_x_0.set_sampling_freq(self.source_rate)

    def get_talkgroup(self):
        return self.talkgroup

    def set_talkgroup(self, talkgroup):
        self.talkgroup = talkgroup

    def get_tone_threshold(self):
        return self.tone_threshold

    def set_tone_threshold(self, tone_threshold):
        self.tone_threshold = tone_threshold
        self.edacs_handle_eot_0.set_tone_threshold(self.tone_threshold)

    def get_track_analog(self):
        return self.track_analog

    def set_track_analog(self, track_analog):
        self.track_analog = track_analog

    def get_track_digital(self):
        return self.track_digital

    def set_track_digital(self, track_digital):
        self.track_digital = track_digital

    def get_voice_threshold(self):
        return self.voice_threshold

    def set_voice_threshold(self, voice_threshold):
        self.voice_threshold = voice_threshold

    def get_baud_rate(self):
        return self.baud_rate

    def set_baud_rate(self, baud_rate):
        self.baud_rate = baud_rate
        self.set_samp_per_sym(self.sink_rate / self.baud_rate)

    def get_samp_per_sym(self):
        return self.samp_per_sym

    def set_samp_per_sym(self, samp_per_sym):
        self.samp_per_sym = samp_per_sym

    def get_fp_sel_index(self):
        return self.fp_sel_index

    def set_fp_sel_index(self, fp_sel_index):
        self.fp_sel_index = fp_sel_index
        self.blks2_selector_1.set_input_index(int(self.fp_sel_index))
