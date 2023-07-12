let midi = null;
let delugeIn = null;
let delugeOut = null;

function onMIDISuccess(midiAccess) {
  console.log("MIDI ready! :D");
  midi = midiAccess; // store in the global (in real usage, would probably keep in an object instance)
}

function onMIDIFailure(msg) {
  console.error(`Failed to get MIDI access :( - ${msg}`);
}

window.addEventListener('load', function() { 
  var navigator = window.navigator;
  if (navigator.requestMIDIAccess) {
    navigator.requestMIDIAccess({ sysex: true }).then( onMIDISuccess, onMIDIFailure );
  } else {
    console.log("why no midi :(");
  }
});

function dobutton() {
  console.log("did button");

   for (const entry of midi.inputs) {
    const input = entry[1];
    console.log(
      `Input port [type:'${input.type}']` +
        ` id:'${input.id}'` +
        ` manufacturer:'${input.manufacturer}'` +
        ` name:'${input.name}'` +
        ` version:'${input.version}'`,
    );

     if (input.name.includes("Deluge MIDI 3")) {
       delugeIn = input;
     }
  }

  for (const entry of midi.outputs) {
    const output = entry[1];
    console.log(
      `Output port [type:'${output.type}'] id:'${output.id}' manufacturer:'${output.manufacturer}' name:'${output.name}' version:'${output.version}'`,
    );

     if (output.name == "Deluge MIDI 3") {
       delugeOut = output;
     }
  }

  if (delugeIn != null && delugeOut != null) {
    alert("found deluge!");
    console.log("huzzah!");
    delugeOut.send([0xf0, 0x7d, 0x01, 0xf7]);
  } else {
    alert("no deluge.");
  }


}
