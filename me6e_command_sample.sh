#################
## for me6e-pt ##
#################
# entry_kind  input_domain  section_device_prefix  input_planeid  input_prefix_len  dst_mac  dst_pt_prefix output_palneid  enable
add  me6e pr  -                  3  64  B0:99:28:1F:51:1E  2001:db8:ffe6::/48     3 enable
add  me6e fp  2001:db8:ffe6::/48 3  48  b0:99:28:1f:59:54  2001:db8:ff57:74::/64  3 enable
add  me6e pr  -                  4  64  00:0A:85:07:01:F5  2001:db8:ffe6::/48     4 enable
add  me6e fp  2001:db8:ffe6::/48 4  48  00:0a:85:07:01:d1  2001:db8:ff57:74::/64  4 enable
