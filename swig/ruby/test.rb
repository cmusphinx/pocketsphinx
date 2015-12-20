#!/usr/bin/ruby

require 'pocketsphinx'

config = Pocketsphinx::Decoder.default_config()
config.set_string('-hmm', '../../model/en-us/en-us')
config.set_string('-dict', '../../model/en-us/cmudict-en-us.dict')
config.set_string('-lm', '../../model/en-us/en-us.lm.bin')
decoder = Pocketsphinx::Decoder.new(config)

decoder.start_utt()

open("../../test/data/goforward.raw") {|f|
  while record = f.read(4096)
    decoder.process_raw(record, false, false)
  end
}
        
decoder.end_utt()
puts decoder.hyp().hypstr()

decoder.seg().each { |seg|
    puts "#{seg.word} #{seg.start_frame} #{seg.end_frame}"
}

decoder.nbest().each(10) { |nbest|
    puts nbest.hyp.hypstr();
}
