# This variable will help provide a master list of all the sources.
# Add new source files here.
set(UNRRDU_SOURCES
  fft.c
  i2w.c
  w2i.c
  1op.c
  2op.c
  3op.c
  affine.c
  about.c
  axdelete.c
  axinfo.c
  basinfo.c
  axinsert.c
  axmerge.c
  axsplit.c
  block.c
  ccadj.c
  ccfind.c
  ccmerge.c
  ccsettle.c
  cmedian.c
  convert.c
  crop.c
  acrop.c
  data.c
  dering.c
  dhisto.c
  dice.c
  dist.c
  env.c
  flip.c
  flotsam.c
  gamma.c
  head.c
  heq.c
  histax.c
  histo.c
  imap.c
  inset.c
  jhisto.c
  join.c
  lut.c
  lut2.c
  make.c
  minmax.c
  cksum.c
  diff.c
  dnorm.c
  vidicon.c
  grid.c
  ilk.c
  ninspect.c
  hack.c
  aabplot.c
  undos.c
  uncmt.c
  mlut.c
  mrmap.c
  pad.c
  permute.c
  privateUnrrdu.h
  project.c
  quantize.c
  resample.c
  reshape.c
  rmap.c
  save.c
  shuffle.c
  slice.c
  sselect.c
  splice.c
  subst.c
  swap.c
  tile.c
  unorient.c
  unquantize.c
  unrrdu.h
  untile.c
  unblock.c
)

target_sources(teem PRIVATE ${UNRRDU_SOURCES})
# This will group all the source files in the VS project by directory
source_group( unrrdu FILES ${UNRRDU_SOURCES} )
