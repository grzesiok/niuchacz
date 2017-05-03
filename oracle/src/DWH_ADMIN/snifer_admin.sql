exec dbms_aqadm.create_queue_table(queue_table => 'qt_core_actions', queue_payload_type => 't_action', multiple_consumers => true);
exec dbms_aqadm.create_queue(queue_name => 'q_core_actions', queue_table => 'qt_core_actions');
exec dbms_aqadm.start_queue(queue_name => 'q_core_actions');