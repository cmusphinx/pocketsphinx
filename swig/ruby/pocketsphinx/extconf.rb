# Loads mkmf which is used to make makefiles for Ruby extensions
require 'mkmf'

# Give it a name
extension_name = 'pocketsphinx'

# The destination
dir_config(extension_name)

# Requirement
pkg_config('pocketsphinx')

# Do the work
create_makefile(extension_name)
