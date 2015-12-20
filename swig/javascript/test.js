var fs = require('fs');

var ps = require('pocketsphinx').ps;

modeldir = "../../model/en-us/"

var config = new ps.Decoder.defaultConfig();
config.setString("-hmm", modeldir + "en-us");
config.setString("-dict", modeldir + "cmudict-en-us.dict");
config.setString("-lm", modeldir + "en-us.lm.bin");
var decoder = new ps.Decoder(config);

fs.readFile("../../test/data/goforward.raw", function(err, data) {
    if (err) throw err;
    decoder.startUtt();
    decoder.processRaw(data, false, false);
    decoder.endUtt();
    console.log(decoder.hyp())

    it = decoder.seg().iter()
    while ((seg = it.next()) != null) {
        console.log(seg.word, seg.startFrame, seg.endFrame);
    }
});

