import yaml
import casperfpga
import time

if __name__=="__main__":

    config_file = 'config.yaml'
    with open(config_file, 'r') as f:
        c = yaml.load(f, Loader=yaml.SafeLoader)

    fpga = casperfpga.CasperFpga('192.168.161.192')
    #fpga.get_system_information()
    time.sleep(2)
    fpga.upload_to_ram_and_program('t47_100g_test2_2025-10-12_1503.fpg')
    time.sleep(4)
    print 'connect done!'
    fpga.write_int('pkt_rst0', 1)
    fpga.write_int('pkt_rst0', 0)

    eth = fpga.gbes['onehundred_gbe0']
    #fpga.write_int('pkt_rst', 1)
    #fpga.write_int('pkt_rst', 0)
    #fpga.write_int('pkt_rst', 2)
    #fpga.write_int('pkt_rst', 0)
    # configure core information
    ip = '10.17.16.60'
    mac = c['arp']['f-engine'][ip]
    eth.configure_core(mac, ip, 60000)

    #eth1 = fpga.gbes['onehundred_gbe1']
    #ip1 = '10.17.16.61'
    #mac1 = c['arp']['f-engine'][ip1]
    #eth1.configure_core(mac1, ip1, 60001)


    # # if ever in doubt, all can be configured from the register memory map
    # fpga.write_int('gmac_reg_mac_address_l_0', (mac & 0xffffffff))
    # fpga.write_int('gmac_reg_mac_address_h_0', (mac >> 32) & 0xffff)

    # set arp table
    for k, v, in c['arp'].items():
        print("Configuring arp values for {:s}".format(k))
        for ip, mac in v.items():
            print(ip, hex(mac))
            eth.set_single_arp_entry(ip, mac)
        print()

    # data starts flowing at this point

    #send data

    #fpga.write_int('tx_dest_port0',60000)
    fpga.write_int('tx_dest_port0',60000)

    #fpga.write('tx_dest_ip0',str(bytearray([10,17,16,10])),offset=0x0)
    dest_ip0=10*(2**24)+17*(2**16)+16*(2**8)+11
    #fpga.write('tx_dest_ip0',str(bytearray([10,17,16,11])),offset=0x0)
    fpga.write_int('tx_dest_ip0',dest_ip0)
    period=256#185
    print(256e6/period*8256*8/1024/1024/1024)
    payload_len=128#140#128+13#105*11	

    #fpga.write_int('send_enable1',1)    

    fpga.write_int('pkt_sim_period',period)
    fpga.write_int('pkt_sim_payload_len',payload_len) 
    fpga.write_int('pkt_sim_enable',1)
    


    print "initial done"
    print eth.print_gbe_core_details()
