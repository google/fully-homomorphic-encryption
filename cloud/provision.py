"""Provisions and destroys TPU resources on GCP."""

from typing import Optional

from core import Core
from core import get_node
from core import stop_node
from core import tpu_client
import google.api_core.exceptions as core_exceptions
from google.cloud import compute_v1
from google.cloud import tpu_v2


def provision_subnet(
    core: Core, name: str, subnet: Optional[str] = None
) -> compute_v1.Subnetwork:
  """Provisions a subnet."""
  if subnet is not None:  # fetch subnet
    return compute_v1.SubnetworksClient().get(
        project=core.project, region=core.region, subnetwork=subnet
    )

  print(f"Provisioning network {name=} ", end=" - ")
  try:
    compute_v1.NetworksClient().insert(
        project=core.project,
        network_resource=compute_v1.Network(
            name=name,
            auto_create_subnetworks=False,
            routing_config=compute_v1.NetworkRoutingConfig(
                routing_mode="REGIONAL"
            ),
        ),
    ).result()
  except core_exceptions.Conflict:
    print("already exists")
  else:
    print("done")
  nw = compute_v1.NetworksClient().get(project=core.project, network=name)

  print(f"Provisioning subnetwork {name=}", end=" - ", flush=True)
  sn_op = None
  try:
    sn_op = compute_v1.SubnetworksClient().insert(
        project=core.project,
        region=core.region,
        subnetwork_resource=compute_v1.Subnetwork(
            name=name,
            network=nw.self_link,
            ip_cidr_range="10.0.0.0/16",
            stack_type="IPV4_ONLY",
        ),
    )
  except core_exceptions.Conflict:
    print("already exists")
  else:
    print("done")

  fw_name = f"{name}-allow-ssh"
  print(f"Provisioning firewall rule {fw_name}", end=" - ", flush=True)
  try:
    compute_v1.FirewallsClient().insert(
        project=core.project,
        firewall_resource=compute_v1.Firewall(
            name=fw_name,
            network=nw.self_link,
            direction="INGRESS",
            allowed=[compute_v1.Allowed(I_p_protocol="tcp", ports=["22"])],
            source_ranges=["0.0.0.0/0"],
            priority=1000,
            log_config=compute_v1.FirewallLogConfig(enable=False),
        ),
    ).result()
  except core_exceptions.Conflict:
    print("already exists")
  else:
    print("done")

  if sn_op is not None:
    sn_op.result()  # wait for subnet to be created
  return compute_v1.SubnetworksClient().get(
      project=core.project, region=core.region, subnetwork=name
  )


def provision(
    core: Core,
    name: str,
    runtime_version: str = None,
    accelerator_type: str = None,
    subnet_url: Optional[str] = None,
    keep_running: bool = False,
) -> None:
  """Provisions a TPU node.

  Args:
    core:
    name: Name of the TPU node.
    runtime_version: TPU runtime version.
    accelerator_type: TPU accelerator type.
    subnet_url: URL of the subnet to use. If not provided, a new subnet will be
      created.
    keep_running: If True, the TPU node will not be stopped after creation.
  """
  if runtime_version is None or accelerator_type is None:
    print("Runtime version and accelerator type are required but not provided")

  subnet = provision_subnet(core, name=name, subnet=subnet_url)

  req = tpu_v2.CreateNodeRequest(
      parent=core.parent,
      node_id=name,
      node=tpu_v2.Node(
          # https://cloud.google.com/php/docs/reference/cloud-tpu/1.1.1/V2.Node
          name=name,
          description="Created automagically for HEIR experiments",
          accelerator_type=accelerator_type,
          runtime_version=runtime_version,
          shielded_instance_config={},
          network_config=tpu_v2.NetworkConfig(
              network=subnet.network,
              subnetwork=subnet.self_link,
              enable_external_ips=True,
              can_ip_forward=True,
          ),
          metadata={
              "startup-script": """
#!/bin/bash
set -eux
pip install -U "jax[tpu]" -f https://storage.googleapis.com/jax-releases/libtpu_releases.html
pip install jax==0.6.0
pip install jaxlib==0.6.0
pip install jaxite
pip install absl-py
""",
          },
          # cidr_block !!!
          # accelerator_config - already supplied accelerator_type ?
          # service_account, scheduling_config, labels, tags, data_disks
      ),
  )

  print(f"Creating TPU VM {name=}")
  tpu_vm_client = tpu_v2.TpuClient()
  try:
    op = tpu_vm_client.create_node(request=req)
  except core_exceptions.ResourceExhausted as e:
    print(f"Resource exhausted: {e}")
    print(
        "Try later or different zone. Available zones:"
        f" {list(core.available_zones().keys())}"
    )
    return

  print("Waiting for operation to complete")
  node = op.result()
  print("TPU node has been created")
  if not keep_running:
    stop_node(node)


def destroy_tpu(core: Core, name: str) -> None:
  print(f"Deleting TPU {name}", end=" - ", flush=True)
  try:
    node = get_node(core.parent, name)
  except core_exceptions.NotFound:
    print("TPU node not found")
    return
  tpu_client().delete_node(name=node.name).result()
  print("done")


def destroy_network(core: Core, name: str) -> None:
  """Deletes the network, subnetwork and firewall rule."""
  try:
    fw_name = f"{name}-allow-ssh"
    print(f"Deleting firewall rule {fw_name}", end=" - ", flush=True)
    compute_v1.FirewallsClient().delete(
        project=core.project, firewall=fw_name
    ).result()
    print("done")
  except core_exceptions.NotFound:
    print("not found")

  try:
    print(f"Deleting subnetwork {name}", end=" - ", flush=True)
    compute_v1.SubnetworksClient().delete(
        project=core.project, region=core.region, subnetwork=name
    ).result()
    print("done")
  except core_exceptions.NotFound:
    print("not found")

  try:
    print(f"Deleting network {name}", end=" - ", flush=True)
    compute_v1.NetworksClient().delete(
        project=core.project, network=name
    ).result()
    print("done")
  except core_exceptions.NotFound:
    print("not found")


def destroy(core: Core, name: str, keep_network: bool = False) -> None:
  try:
    destroy_tpu(core, name)
  finally:
    if not keep_network:
      destroy_network(core, name)
