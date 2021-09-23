# -----------------------------
# Options affecting formatting.
# -----------------------------
with section("format"):

  # How wide to allow formatted cmake files
  line_width = 160

  # How many spaces to tab for indent
  tab_size = 4

  # If a statement is wrapped to more than one line, than dangle the closing
  # parenthesis on its own line.
  dangle_parens = False

  # Format command names consistently as 'lower' or 'upper' case
  command_case = 'lower'

# ------------------------------------------------
# Options affecting comment reflow and formatting.
# ------------------------------------------------
with section("markup"):

  # enable comment markup parsing and reflow
  enable_markup = False
