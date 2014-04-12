Build instructions
------------------
<ul>
<li> Build and install ct-sdl2</li>
<li> Build mame. Only the executable (mame) builds for now. File chooser is not working yet.</li>
</ul>

<pre>
git clone https://github.com/fruit-bat/ct-mame4allSdl2.git
cd ct-mame4allSdl2
make -j2 -f makefile.ct mame
</pre>
