#################
## for m46e-pt ##
#################
# entry_kind  input_domain  section_device_prefix  input_planeid  input_prefix_len  dst_addr/prefix_len  dst_pt_prefix/prefix_len  output_palneid  enable
add  m46e pr  -                   0:1  64  192.168.1.0/24  2001:db8:ff46::/48       0:0:1 enable
add  m46e fp  2001:db8:ff46::/48  0:1  48  192.168.5.0/24  2001:db8:ff57:73::/64    0:1   enable
add  m46e pr  -                   0:2  64  192.168.2.0/24  2001:db8:ff46::/48       0:0:2 enable
add  m46e fp  2001:db8:ff46::/48  0:2  48  192.168.6.0/24  2001:db8:ff57:73::/64    0:2   enable
