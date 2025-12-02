#!/usr/bin/python3

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import OVSController
from mininet.log import setLogLevel, info
from mininet.cli import CLI

class LeafSpine(Topo):
    """
    Custom Scalable Leaf–Spine Topology
    leaf_count   = number of Leaf switches
    spine_count  = number of Spine switches (switch radix scale)
    hosts_per_leaf = number of hosts under each leaf switch
    """
    def build(self, leaf_count=2, spine_count=2, hosts_per_leaf=2):

        spine = []
        leaf = []

        info("*** Creating Spine Switches\n")
        for i in range(spine_count):
            sw = self.addSwitch(f"s_spine{i+1}")
            spine.append(sw)

        info("*** Creating Leaf Switches\n")
        for i in range(leaf_count):
            sw = self.addSwitch(f"s_leaf{i+1}")
            leaf.append(sw)

        info("*** Connecting Leaf→Spine links\n")
        for lsw in leaf:
            for ssw in spine:
                self.addLink(lsw, ssw)

        info("*** Adding hosts and connecting to leaf switches\n")
        host_id = 1
        for lsw in leaf:
            for _ in range(hosts_per_leaf):
                host = self.addHost(f"h{host_id}")
                self.addLink(host, lsw)
                host_id += 1


def run_topology():
    topo = LeafSpine(leaf_count=3, spine_count=2, hosts_per_leaf=2)

    net = Mininet(topo=topo, controller=OVSController)
    net.start()

    info("*** Leaf–Spine Topology Ready\n")
    CLI(net)
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    run_topology()
