# KU_IPC
  - System V message_queue를 linux kernel에서 사용할수있도록 구현하였습니다.
  
  
  - user_application에서 메세지를 보내어 linux kernel에있는 message_queue에 저장할수있으며
  
  
  - 반대로 linux kernel에서 메세지를 받아 user_application에서 사용할수있습니다.
  
  
  - 10개의 메세지큐가존재하며 각큐를사용할때는 key를받아 사용합니다 이미 user_application에서 key가사용중이라면
    사용할수없습니다.
