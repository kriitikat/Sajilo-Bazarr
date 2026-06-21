

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";

CREATE TABLE `categories` (
  `id` int(11) NOT NULL,
  `category_name` varchar(100) DEFAULT NULL,
  `description` text DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;



INSERT INTO `categories` (`id`, `category_name`, `description`) VALUES
(1, 'Groceries', 'Daily grocery products and essential kitchen items'),
(2, 'Vegetables', 'Fresh vegetables supplied daily'),
(3, 'Fruits', 'Fresh seasonal fruits and imported fruits'),
(4, 'Dairy', 'Milk products and dairy related items'),
(5, 'Cosmetics', 'Beauty and personal care products'),
(6, 'Cleaning', 'Cleaning and sanitation products'),
(7, 'Household', 'Daily usable household products'),
(8, 'Snacks', 'Quick snack and packaged food items'),
(9, 'Beverages', 'Cold drinks, tea, coffee and juices'),
(10, 'Bakery', 'Fresh bakery products'),
(11, 'Stationery', 'Office and school stationery items');

-- --------------------------------------------------------

--
-- Table structure for table `information`
--

CREATE TABLE `information` (
  `id` int(11) NOT NULL,
  `first_name` varchar(100) DEFAULT NULL,
  `last_name` varchar(100) DEFAULT NULL,
  `username` varchar(150) DEFAULT NULL,
  `role` enum('admin','staff','frontdesk') NOT NULL,
  `email` varchar(150) DEFAULT NULL,
  `phone` varchar(20) DEFAULT NULL,
  `picture` varchar(255) DEFAULT NULL,
  `password` varchar(255) DEFAULT NULL,
  `status` enum('approved','pending') DEFAULT 'pending'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `information`
--

INSERT INTO `information` (`id`, `first_name`, `last_name`, `username`, `role`, `email`, `phone`, `picture`, `password`, `status`) VALUES
(1, 'Anushka', 'Sigdel', 'Anushka_Sigdel_Admin', 'admin', 'anushkasigdel50@gmail.com', '9761468969', 'anushka.jpg', '202cb962ac59075b964b07152d234b70', 'approved'),
(2, 'Suyeshna', 'Shrestha', 'Suyeshna_Shrestha_Staff', 'staff', 'suyeshna@gmail.com', '9801111111', 'suyeshna.jpg', 'Suyeshna\r\n', 'approved'),
(3, 'Kritika', 'Gurung', 'Kritika_Gurung_Staff', 'staff', 'kritika@gmail.com', '9802222222', 'kritika.jpg', 'de9bf5643eabf80f4a56fda3bbb84483', 'approved'),
(4, 'Milan', 'Tamang', 'Milan_Tamang_Staff', 'staff', 'milan@gmail.com', '9803333333', 'milan.jpg', 'de9bf5643eabf80f4a56fda3bbb84483', 'approved'),
(5, 'Ritika', 'Karki', 'Ritika_Karki_Staff', 'staff', 'ritika@gmail.com', '9804444444', 'ritika.jpg', 'de9bf5643eabf80f4a56fda3bbb84483', 'approved'),
(6, 'Roshan', 'Basnet', 'Roshan_Basnet_Staff', 'staff', 'roshan@gmail.com', '9805555555', 'roshan.jpg', 'de9bf5643eabf80f4a56fda3bbb84483', 'approved'),
(7, 'Prisha', 'Adhikari', 'Prisha_Adhikari_Frontdesk', 'frontdesk', 'prisha@gmail.com', '9811111111', 'prisha.jpg', 'prisha', 'approved'),
(8, 'Aayush', 'Thapa', 'Aayush_Thapa_Frontdesk', 'frontdesk', 'aayush@gmail.com', '9812222222', 'aayush.jpg', '55a85a37ee421480991c49c13316dde4', 'approved'),
(9, 'Shreekrishna', 'Mishra', 'shreekrishna_mishra_staff', 'staff', 'skjfksj@gmail.com', '9849803850', NULL, '827ccb0eea8a706c4c34a16891f84e7b', 'pending'),
(10, 'Anushka', 'Sigdel', 'anushka_sigdel_frontdesk', 'frontdesk', 'gjhvjhg@gmail.com', '646858758', NULL, '202cb962ac59075b964b07152d234b70', 'pending'),
(11, 'Kismat', 'ThapaMagar', 'kismat_thapamagar_staff', 'staff', 'skjfksj@gmail.com', '9849803850', NULL, '81dc9bdb52d04dc20036dbd8313ed055', 'pending');

-- --------------------------------------------------------

--
-- Table structure for table `products`
--

CREATE TABLE `products` (
  `id` int(11) NOT NULL,
  `product_name` varchar(150) DEFAULT NULL,
  `category` varchar(100) DEFAULT NULL,
  `unit` varchar(50) DEFAULT NULL,
  `price` decimal(10,2) DEFAULT NULL,
  `stock` int(11) DEFAULT NULL,
  `expiry_date` date DEFAULT NULL,
  `status` varchar(50) DEFAULT NULL,
  `supplier` varchar(150) DEFAULT NULL,
  `sku` varchar(100) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `products`
--

INSERT INTO `products` (`id`, `product_name`, `category`, `unit`, `price`, `stock`, `expiry_date`, `status`, `supplier`, `sku`) VALUES
(1, 'Rice', 'Grains', 'kg', 90.00, 5, '2027-01-10', 'Low Stock', 'Nepal Agro', 'SKU001'),
(2, 'Sugar', 'Groceries', 'kg', 120.00, 80, '2027-05-01', 'In Stock', 'Sweet Suppliers', 'SKU002'),
(3, 'Salt', 'Groceries', 'kg', 25.00, 200, '2028-03-15', 'High Stock', 'Everest Salt', 'SKU003'),
(4, 'Potato', 'Vegetables', 'kg', 60.00, 30, '2026-06-15', 'Low Stock', 'Fresh Farm', 'SKU004'),
(5, 'Tomato', 'Vegetables', 'kg', 80.00, 45, '2026-06-11', 'In Stock', 'Fresh Farm', 'SKU005'),
(6, 'Onion', 'Vegetables', 'kg', 70.00, 25, '2026-06-20', 'Low Stock', 'Veggie Nepal', 'SKU006'),
(7, 'Milk', 'Dairy', 'litre', 110.00, 60, '2026-06-08', 'In Stock', 'Dairy Nepal', 'SKU007'),
(8, 'Cheese', 'Dairy', 'pieces', 350.00, 15, '2026-07-01', 'Low Stock', 'Dairy Nepal', 'SKU008'),
(9, 'Butter', 'Dairy', 'pieces', 250.00, 40, '2026-08-01', 'In Stock', 'Dairy Nepal', 'SKU009'),
(10, 'Yogurt', 'Dairy', 'pieces', 90.00, 65, '2026-06-09', 'In Stock', 'Fresh Dairy', 'SKU010'),
(11, 'Toothpaste', 'Cosmetics', 'pieces', 140.00, 90, '2028-01-01', 'High Stock', 'Colgate Nepal', 'SKU011'),
(12, 'Soap', 'Cosmetics', 'pieces', 60.00, 150, '2028-01-01', 'High Stock', 'Clean Nepal', 'SKU012'),
(13, 'Shampoo', 'Cosmetics', 'bottle', 450.00, 70, '2027-10-10', 'In Stock', 'Beauty Nepal', 'SKU013'),
(14, 'Face Wash', 'Cosmetics', 'pieces', 320.00, 20, '2027-11-12', 'Low Stock', 'Glow Nepal', 'SKU014'),
(15, 'Body Lotion', 'Cosmetics', 'bottle', 500.00, 55, '2027-12-01', 'In Stock', 'Beauty Nepal', 'SKU015'),
(16, 'Dish Wash', 'Cleaning', 'bottle', 180.00, 35, '2027-09-09', 'In Stock', 'Sparkle Suppliers', 'SKU016'),
(17, 'Detergent', 'Cleaning', 'kg', 220.00, 95, '2027-07-07', 'High Stock', 'Sparkle Suppliers', 'SKU017'),
(18, 'Floor Cleaner', 'Cleaning', 'bottle', 300.00, 45, '2027-09-01', 'In Stock', 'Clean Nepal', 'SKU018'),
(19, 'Glass Cleaner', 'Cleaning', 'bottle', 280.00, 15, '2027-12-12', 'Low Stock', 'Sparkle Suppliers', 'SKU019'),
(21, 'Bucket', 'Household', 'pieces', 250.00, 60, '2030-01-01', 'In Stock', 'Home Nepal', 'SKU021'),
(22, 'Plate', 'Household', 'pieces', 120.00, 85, '2030-01-01', 'High Stock', 'Kitchen Nepal', 'SKU022'),
(23, 'Cup', 'Household', 'pieces', 100.00, 100, '2030-01-01', 'High Stock', 'Kitchen Nepal', 'SKU023'),
(24, 'Biscuit', 'Snacks', 'pieces', 30.00, 170, '2026-09-01', 'High Stock', 'Snack Nepal', 'SKU024'),
(25, 'Chips', 'Snacks', 'pieces', 50.00, 130, '2026-08-01', 'High Stock', 'Snack Nepal', 'SKU025'),
(26, 'Noodles', 'Snacks', 'pieces', 25.00, 220, '2027-01-01', 'High Stock', 'Wai Wai', 'SKU026'),
(27, 'Juice', 'Beverages', 'bottle', 120.00, 65, '2026-10-01', 'In Stock', 'Juice Nepal', 'SKU027'),
(28, 'Coke', 'Beverages', 'bottle', 110.00, 140, '2027-01-01', 'High Stock', 'Coca Cola Nepal', 'SKU028'),
(29, 'Fanta', 'Beverages', 'bottle', 110.00, 120, '2027-01-01', 'High Stock', 'Coca Cola Nepal', 'SKU029'),
(30, 'Sprite', 'Beverages', 'bottle', 110.00, 115, '2027-01-01', 'High Stock', 'Coca Cola Nepal', 'SKU030'),
(31, 'Tea', 'Beverages', 'box', 355.00, 40, '2027-04-04', 'In Stock', 'Tea Nepal', 'SKU031'),
(32, 'Coffee', 'Beverages', 'box', 550.00, 28, '2027-06-06', 'Low Stock', 'Coffee House', 'SKU032'),
(33, 'Bread', 'Bakery', 'pieces', 55.00, 22, '2026-06-06', 'Low Stock', 'Bakery Nepal', 'SKU033'),
(34, 'Cake', 'Bakery', 'pieces', 850.00, 8, '2026-06-07', 'Low Stock', 'Bakery Nepal', 'SKU034'),
(35, 'Eggs', 'Dairy', 'dozen', 220.00, 55, '2026-06-09', 'In Stock', 'Poultry Nepal', 'SKU035'),
(36, 'Chicken', 'Meat', 'kg', 480.00, 35, '2026-06-06', 'In Stock', 'Fresh Meat', 'SKU036'),
(37, 'Fish', 'Meat', 'kg', 620.00, 14, '2026-06-06', 'Low Stock', 'Fresh Meat', 'SKU037'),
(38, 'Apple', 'Fruits', 'kg', 180.00, 50, '2026-06-15', 'In Stock', 'Fruit Nepal', 'SKU038'),
(39, 'Banana', 'Fruits', 'dozen', 140.00, 45, '2026-06-10', 'In Stock', 'Fruit Nepal', 'SKU039'),
(40, 'Orange', 'Fruits', 'kg', 220.00, 38, '2026-06-12', 'In Stock', 'Fruit Nepal', 'SKU040'),
(41, 'Watermelon', 'Fruits', 'pieces', 350.00, 10, '2026-06-08', 'Low Stock', 'Fruit Nepal', 'SKU041'),
(42, 'Cooking Oil', 'Groceries', 'litre', 340.00, 90, '2027-07-07', 'High Stock', 'Oil Nepal', 'SKU042'),
(43, 'Pulses', 'Groceries', 'kg', 160.00, 75, '2027-09-09', 'In Stock', 'Agro Nepal', 'SKU043'),
(44, 'Flour', 'Groceries', 'kg', 85.00, 95, '2027-10-10', 'High Stock', 'Agro Nepal', 'SKU044'),
(45, 'Corn Flakes', 'Breakfast', 'box', 420.00, 25, '2027-08-08', 'Low Stock', 'Breakfast Nepal', 'SKU045'),
(46, 'Oats', 'Breakfast', 'box', 380.00, 32, '2027-07-07', 'In Stock', 'Breakfast Nepal', 'SKU046'),
(47, 'Notebook', 'Stationery', 'pieces', 80.00, 110, '2030-01-01', 'High Stock', 'Stationery Nepal', 'SKU047'),
(48, 'Pen', 'Stationery', 'pieces', 20.00, 250, '2030-01-01', 'High Stock', 'Stationery Nepal', 'SKU048'),
(49, 'Marker', 'Stationery', 'pieces', 60.00, 70, '2030-01-01', 'In Stock', 'Stationery Nepal', 'SKU049'),
(50, 'Stapler', 'Stationery', 'pieces', 180.00, 18, '2030-01-01', 'Low Stock', 'Stationery Nepal', 'SKU050'),
(51, 'Guitar', 'Music', 'piece', 5000.00, 25, '2026-08-01', 'In Stock', 'Farm', 'M3000');

-- --------------------------------------------------------

--
-- Table structure for table `suppliers`
--

CREATE TABLE `suppliers` (
  `id` int(11) NOT NULL,
  `supplier_name` varchar(150) DEFAULT NULL,
  `email` varchar(150) DEFAULT NULL,
  `phone` varchar(20) DEFAULT NULL,
  `product` varchar(150) DEFAULT NULL,
  `supplied_categories` varchar(255) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `suppliers`
--

INSERT INTO `suppliers` (`id`, `supplier_name`, `email`, `phone`, `product`, `supplied_categories`) VALUES
(1, 'Fresh Farm', 'freshfarm@gmail.com', '9801111000', 'Vegetables', 'Vegetables'),
(2, 'Dairy Nepal', 'dairy@gmail.com', '9801111001', 'Milk', 'Dairy'),
(3, 'Snack Nepal', 'snack@gmail.com', '9801111002', 'Chips', 'Snacks'),
(4, 'Fruit Nepal', 'fruit@gmail.com', '9801111003', 'Apple', 'Fruits'),
(5, 'Kitchen Nepal', 'kitchen@gmail.com', '9801111004', 'Plates', 'Household');

-- --------------------------------------------------------

--
-- Table structure for table `tasks`
--

CREATE TABLE `tasks` (
  `id` int(11) NOT NULL,
  `staff_name` varchar(150) DEFAULT NULL,
  `task_title` varchar(255) DEFAULT NULL,
  `task_description` text DEFAULT NULL,
  `deadline` date DEFAULT NULL,
  `priority` varchar(50) DEFAULT NULL,
  `status` varchar(50) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `categories`
--
ALTER TABLE `categories`
  ADD PRIMARY KEY (`id`);

--
-- Indexes for table `information`
--
ALTER TABLE `information`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `username` (`username`);

--
-- Indexes for table `products`
--
ALTER TABLE `products`
  ADD PRIMARY KEY (`id`);

--
-- Indexes for table `suppliers`
--
ALTER TABLE `suppliers`
  ADD PRIMARY KEY (`id`);

--
-- Indexes for table `tasks`
--
ALTER TABLE `tasks`
  ADD PRIMARY KEY (`id`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `categories`
--
ALTER TABLE `categories`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=12;

--
-- AUTO_INCREMENT for table `information`
--
ALTER TABLE `information`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=15;

--
-- AUTO_INCREMENT for table `products`
--
ALTER TABLE `products`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=52;

--
-- AUTO_INCREMENT for table `suppliers`
--
ALTER TABLE `suppliers`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=6;

--
-- AUTO_INCREMENT for table `tasks`
--
ALTER TABLE `tasks`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
